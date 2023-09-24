#include "ASMbuilder.h"

namespace dark::ASM {

/* Pre-scanning part. */
void ASMbuilder::pre_scanning(IR::function *__func) {
    size_t __count = 0;
    IR::call_stmt *__flag = nullptr; /* Whether tail-callable */
    for(auto __p : __func->stmt) {
        for(auto __stmt : __p->stmt) {
            if (auto *__ret = dynamic_cast <IR::return_stmt *> (__stmt);
                __ret != nullptr && __flag != nullptr) {
                /* Try to tail-call. */
                if (!__ret->rval || __ret->rval == __flag->dest) {
                    tail_call_set.insert(__flag);
                    tail_call_set.insert(__ret);
                } continue;
            }

            __flag = nullptr;
            for(auto __use : __stmt->get_use()) ++use_map[__use].count;
            if (auto __def = __stmt->get_def()) use_map[__def].def = __stmt;
            if (auto __get = dynamic_cast <IR::get_stmt *> (__stmt)) {
                /* The offset is a certain value. */
                if (auto __idx = dynamic_cast <IR::integer_constant *> (__get->idx)) {
                    size_t __offset = __idx->value * (--__get->dst->type).size();
                    if (__get->mem != __get->NPOS)
                        __offset += __get->mem * IR::MxPTRSIZE;
                    getelement_map[__get->dst] = {
                        dynamic_cast <IR::non_literal *> (__get->src), __offset
                    };
                }
            } else if(auto __alloc = dynamic_cast <IR::allocate_stmt *> (__stmt)) {
                offset_map[__alloc->dest] = top_asm->allocate(__alloc->dest);
            } else if(auto __call = dynamic_cast <IR::call_stmt *> (__stmt)) {
                top_asm->update_size(__call->func->args.size());
                __flag = __call;
            } else if (auto __br = dynamic_cast <IR::branch_stmt *> (__stmt)) {
                --use_map[__br->cond].count;
            }
        }
    }

    top_asm->vir_map.reserve(__count);
    top_asm->init_arg_offset();
}


void ASMbuilder::create_entry(IR::function *__func) {
    top_block = get_block(__func->stmt[0]);
    for (size_t i = 0 ; i < __func->args.size() ; ++i) {
        auto *__arg = __func->args[i]; 
        if (__arg->is_dead()) continue; /* Dead arguments. */
        auto *__dst = get_virtual(__arg);

        if (i < 8) {
            top_asm->dummy_def.push_back({pointer_address {__dst,0}});
        } else { /* Excessive arguments in stack. */
            top_block->emplace_back(new load_memory {
                load_memory::WORD, __dst,
                pointer_address {get_physical(2),ssize_t(i - 8) << 2}
            });
        }
    }
}


void ASMbuilder::visitBlock(IR::block_stmt *__block) {
    top_stmt  = __block;
    top_block = get_block(__block);
    top_asm->emplace_back(top_block);
    for(auto __p : __block->stmt) visit(__p);
}


void ASMbuilder::visitFunction(IR::function *__func) {
    top_func = __func;
    top_asm  = get_function(__func);
    create_entry(__func);
    pre_scanning(__func);
    for(auto __p : __func->stmt) visitBlock(__p);
    for(auto &__ref : phi_map) {
        auto &__name = __ref.first->name;
        top_asm->emplace_back(top_block = __ref.first);
        resolve_phi(__ref.second);
    }
    use_map.clear();
    phi_map.clear();
    temp_map.clear();
    offset_map.clear();
    branch_map.clear();
    tail_call_set.clear();
    getelement_map.clear();
}


void ASMbuilder::visitInit(IR::initialization *init) {
    auto *__var = init->dest;
    auto *__lit = init->lite;
    if (auto __str = dynamic_cast <IR::string_constant *> (__lit))
        global_info.rodata_list.push_back(*init);
    else
        global_info.data_list.push_back(*init);
}


void ASMbuilder::visitCompare(IR::compare_stmt *__stmt) {
    if (use_map.at(__stmt->dest).count <= 0) return;
    /* Ensurance : no constant on lhs. */
    auto *__lhs = dynamic_cast <IR::literal *> (__stmt->lvar);
    auto *__rhs = dynamic_cast <IR::literal *> (__stmt->rvar);

    /* Constant folding! */
    if (__lhs && __rhs) {
        bool __ans = 0;
        auto __lval = __lhs->get_constant_value();
        auto __rval = __rhs->get_constant_value();
        switch (__stmt->op) {
            case __stmt->EQ: __ans = __lval == __rval; break;
            case __stmt->NE: __ans = __lval != __rval; break;
            case __stmt->LT: __ans = __lval <  __rval; break;
            case __stmt->LE: __ans = __lval <= __rval; break;
            case __stmt->GT: __ans = __lval >  __rval; break;
            case __stmt->GE: __ans = __lval >= __rval; break;
        }
        /* Load immediate -> addi zero  */
        return top_block->emplace_back(new arith_immediat {
            arith_base::ADD, get_physical(0), __ans, get_virtual(__stmt->dest)
        });
    }

    /* Formalize those cases. */
    switch(__stmt->op) {
        case __stmt->EQ:
        case __stmt->NE:
            /* Swap constant to right if possible. */
            if (__lhs) {
                std::swap(__lhs,__rhs);
                std::swap(__stmt->lvar,__stmt->rvar);
            } break;

        case __stmt->LE:
            std::swap(__lhs,__rhs);
            std::swap(__stmt->lvar,__stmt->rvar);
            __stmt->op = __stmt->GE;
            break;

        case __stmt->GT:
            std::swap(__lhs,__rhs);
            std::swap(__stmt->lvar,__stmt->rvar);
            __stmt->op = __stmt->LT;
    }

    /* Boolean true of false. */
    if (__stmt->lvar->get_value_type().size() == 1) {
        if (__rhs) {
            if (__rhs->get_constant_value() == __stmt->op) {
                /* x == 0 || x != 1 */
                top_block->emplace_back(new bool_convert {
                    bool_convert::EQZ,
                    force_register(__stmt->lvar),
                    get_virtual(__stmt->dest)
                });
            } else {
                /* x == 1 || x != 0 */
                top_block->emplace_back(new bool_convert {
                    bool_convert::NEZ,
                    force_register(__stmt->lvar),
                    get_virtual(__stmt->dest)
                });
            } 
        } else {
            auto *__vir = get_virtual(__stmt->dest);
            auto *__tmp = __stmt->op == __stmt->EQ ? create_virtual() : __vir;
            top_block->emplace_back(new arith_register {
                arith_base::XOR,
                force_register(__stmt->lvar),
                force_register(__stmt->rvar),
                __tmp
            });
            /* x == y --> x != y */
            if (__stmt->op == __stmt->EQ) {
                top_block->emplace_back(new bool_convert {
                    bool_convert::EQZ, __tmp, __vir
                });
            }
        } return;
    }

    /* X < C */
    if (__stmt->op == __stmt->LT && __rhs) {
        return top_block->emplace_back(new slt_immediat {
            force_register(__stmt->lvar), __rhs->get_constant_value(),
            get_virtual(__stmt->dest)
        });
    }

    /* C >= X --> X < C + 1 */
    if (__stmt->op == __stmt->GE && __lhs) {
        return top_block->emplace_back(new slt_immediat {
            force_register(__stmt->rvar), __lhs->get_constant_value() + 1,
            get_virtual(__stmt->dest)
        });
    }

    /* X >= C --> !(X < C) */
    if (__stmt->op == __stmt->GE && __rhs) {
        auto *__tmp = create_virtual();
        top_block->emplace_back(new slt_immediat {
            force_register(__stmt->lvar),
            __rhs->get_constant_value(), __tmp
        });
        return top_block->emplace_back(new bool_convert {
            bool_convert::EQZ, __tmp, get_virtual(__stmt->dest)
        });
    }

    /* X xor C case.  */
    if (__rhs) { /* This may go wrong when compare 2 strings. */
        auto *__vir = get_virtual(__stmt->dest);
        auto *__tmp = __stmt->op == __stmt->EQ ? create_virtual() : __vir;
        top_block->emplace_back(new arith_immediat {
            arith_base::XOR,
            force_register(__stmt->lvar),
            __rhs->get_constant_value(), __tmp
        });
        /* x == y --> x != y */
        if (__stmt->op == __stmt->EQ) {
            top_block->emplace_back(new bool_convert {
                bool_convert::EQZ, __tmp, __vir
            });
        } return;
    }

    auto *__lval = force_register(__stmt->lvar);
    auto *__rval = force_register(__stmt->rvar);
    auto *__dest = get_virtual(__stmt->dest);

    bool __not = false;
    if (__stmt->op == __stmt->EQ) {
        __not = true; __stmt->op = __stmt->NE;
    }
    if (__stmt->op == __stmt->GE) {
        __not = true; __stmt->op = __stmt->LT;
    }
    auto *__temp = __not ? create_virtual() : __dest;

    if (__stmt->op == __stmt->LT) {
        top_block->emplace_back(new slt_register {
            __lval,__rval,__temp
        });
    } else { /* Not equal case. */
        top_block->emplace_back(new arith_register {
            arith_base::XOR,__lval,__rval,__temp
        });
    }

    if (__not) {
        top_block->emplace_back(new bool_convert {
            bool_convert::EQZ,__temp,__dest
        });
    }
}


void ASMbuilder::visitBinary(IR::binary_stmt *__stmt) {
    /* Ensurance : no constant on lhs. */
    auto *__lhs = dynamic_cast <IR::integer_constant *> (__stmt->lvar);
    auto *__rhs = dynamic_cast <IR::integer_constant *> (__stmt->rvar);

    /* Constant folding! */
    if (__lhs && __rhs) {
        int __ans = 0;
        switch (__stmt->op) {
            case __stmt->ADD: __ans = __lhs->value + __rhs->value; break;
            case __stmt->SUB: __ans = __lhs->value - __rhs->value; break;
            case __stmt->MUL: __ans = __lhs->value * __rhs->value; break;
            case __stmt->SDIV:
                __ans = !__rhs->value ? 0 : __lhs->value / __rhs->value; break;
            case __stmt->SREM:
                __ans = !__rhs->value ? 0 : __lhs->value % __rhs->value; break;
            case __stmt->SHL:  __ans = __lhs->value << __rhs->value; break;
            case __stmt->ASHR: __ans = __lhs->value >> __rhs->value; break;
            case __stmt->AND:  __ans = __lhs->value & __rhs->value; break;
            case __stmt->OR:   __ans = __lhs->value | __rhs->value; break;
            case __stmt->XOR:  __ans = __lhs->value ^ __rhs->value; break;
        }
        /* Load immediate -> addi zero  */
        return top_block->emplace_back(new arith_immediat {
            arith_base::ADD, get_physical(0), __ans, get_virtual(__stmt->dest)
        });
    }

    auto *__lval = force_register(__stmt->lvar);
    auto *__dest = get_virtual(__stmt->dest);

    if (__rhs) { /* Use immediate command instead. */
        if (__rhs->value == 0 && __stmt->op == __stmt->ADD)
            return top_block->emplace_back(new move_register {__lval,__dest});

        switch (__stmt->op) {
            case __stmt->ADD:
            case __stmt->SHL:
            case __stmt->ASHR:
            case __stmt->AND:
            case __stmt->OR:
            case __stmt->XOR:
                return top_block->emplace_back(new arith_immediat {
                    static_cast <decltype(arith_base::op)> (__stmt->op),
                    __lval, __rhs->value, __dest
                });
        }
    }

    auto *__rval = force_register(__stmt->rvar);

    top_block->emplace_back(new arith_register {
        static_cast <decltype(arith_base::op)> (__stmt->op),
        __lval, __rval, __dest
    });
}


void ASMbuilder::visitJump(IR::jump_stmt *__stmt) {
    top_block->emplace_back(new jump_expression  {
        get_edge(top_stmt,__stmt->dest)
    });
}


void ASMbuilder::visitBranch(IR::branch_stmt *__stmt) {
    auto &__ref = use_map.at(__stmt->cond);
    auto *__br  = dynamic_cast <IR::compare_stmt *> (__ref.def); 
    if (__ref.count > 0 || !__br) { 
        return top_block->emplace_back(new branch_expression {
            force_register(__stmt->cond),
            get_edge(top_stmt,__stmt->br[0]),
            get_edge(top_stmt,__stmt->br[1])
        });
    } else {
        return top_block->emplace_back(new branch_expression {
            static_cast <decltype (branch_expression::op)> (__br->op),
            force_register(__br->lvar),
            force_register(__br->rvar),
            get_edge(top_stmt,__stmt->br[0]),
            get_edge(top_stmt,__stmt->br[1])
        });
    }
}


void ASMbuilder::visitCall(IR::call_stmt *__stmt) {
    function_node *__call = nullptr;
    if (__stmt->func->is_builtin > 10) {
        auto *__func = get_function(__stmt->func);
        if (__func->name == "__print__") {

        } else if (__func->name == "__printInt__") {

        } else if (__func->name == "__printlnInt__") {

        } else if (__func->name == "__String_parseInt__") {

        } else if (__func->name == "__String_ord__") {

        } else if (__func->name == "__String_parseInt__") {

        } else if (__func->name == "__getString__") {

        } else if (__func->name == "__getInt__") {

        } else if (__func->name == "__toString__") {

        } else if (__func->name == "__Array_size__") {

        } else if (__func->name == "__new_array1__") {

        } else if (__func->name == "__new_array4__") {

        } else {
            __call = new call_function {__func,top_asm};
        }
    } else {
        __call = new call_function {get_function(__stmt->func),top_asm};
    }

    /* Tail call doesn't require saving ra. */
    if (tail_call_set.count(__stmt)) {
        __call->op   = call_function::TAIL;
        __call->dest = nullptr;
    } else { /* Non-tail call requires saving ra. */
        top_asm->save_ra = true;
        __call->dest = __stmt->dest && use_map[__stmt->dest].count > 0 ?
            get_virtual(__stmt->dest) : nullptr;
    }

    for (size_t i = 0 ; i < __stmt->args.size() ; ++i) {
        auto *__arg = __stmt->func->args[i];
        /* Do nothing to dead arguments. */
        if (__arg->is_dead()) continue;

        if (i < 8) { /* Dummy reordering of those arguments. */
            __call->dummy_use.push_back(get_value(__stmt->args[i]));
        } else { /* Excessive arguments stored to stack. */
            top_block->emplace_back(new store_memory {
                memory_base::WORD, force_register(__stmt->args[i]),
                pointer_address{ get_physical(2), ssize_t(i - 8) << 2 }
            });
        }
    }

    top_block->emplace_back(__call);
}


void ASMbuilder::visitLoad(IR::load_stmt *__stmt) {
    auto __type = __stmt->dst->type.size() == 4 ?
        memory_base::WORD : memory_base::BYTE;
    top_block->emplace_back(new load_memory {
        __type, get_virtual(__stmt->dst), get_value(__stmt->src)
    });
}


void ASMbuilder::visitStore(IR::store_stmt *__stmt) {
    auto __type = __stmt->src->get_value_type().size() == 4 ?
        memory_base::WORD : memory_base::BYTE;
    /* Must special judge on store_memory. */
    /* When generating final ASM, we should reassign a
       temporary as address if necessary. */
    top_block->emplace_back(new store_memory {
        __type, force_register(__stmt->src), get_value(__stmt->dst)
    });
}


void ASMbuilder::visitReturn(IR::return_stmt *__stmt) {
    if (tail_call_set.count(__stmt)) return;
    top_block->emplace_back(new ret_expression {
        top_asm, __stmt->rval ? force_register(__stmt->rval) : nullptr
    });
}


void ASMbuilder::visitAlloc(IR::allocate_stmt *__stmt) {}


void ASMbuilder::visitGet(IR::get_stmt *__stmt) {
    /* The distance of 2 pointed to objects. */
    size_t __unit = (--__stmt->dst->type).size();

    /* The offset is a certain value. */
    if (auto __idx = dynamic_cast <IR::integer_constant *> (__stmt->idx)) return;

    if ((__unit != 4 && __unit != 1) ||  __stmt->mem != __stmt->NPOS)
        runtime_assert("Not supported!");

    auto *__idx = force_register(__stmt->idx);
    auto *__src = force_register(__stmt->src);
    auto *__dst = force_register(__stmt->dst);

    if (__unit == 4) {
        auto *__vir = create_virtual();
        top_block->emplace_back(new arith_immediat {
            arith_base::SLL,__idx,2,__vir
        });
        __idx = __vir;
    }

    top_block->emplace_back(new arith_register {
        arith_base::ADD, __idx, __src,__dst
    });

}


void ASMbuilder::visitPhi(IR::phi_stmt *__stmt) {
    auto *__dest = get_virtual(__stmt->dest);
    auto *__temp = top_block;

    for(auto [__value,__label] : __stmt->cond) {
        top_block = get_edge(__label,top_stmt);
        auto &__ref = phi_map[top_block];
        __ref.def.push_back(__dest);
        __ref.use.push_back(get_value(__value));
    }

    top_block = __temp;
}


void ASMbuilder::visitUnreachable(IR::unreachable_stmt *__stmt) {
    return top_block->emplace_back(new ret_expression {top_asm,nullptr});
}


}