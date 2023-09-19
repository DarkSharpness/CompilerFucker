#include "ASMbuilder.h"

namespace dark::ASM {


void ASMbuilder::create_entry(IR::function *__func) {
    top_block = get_virtual_block <8> (__func->stmt.front());
    top_asm->emplace_back(top_block);
    for (size_t i = 0 ; i < __func->args.size() ; ++i) {
        auto *__dest = get_virtual(__func->args[i]);
        if (i < 8) {
            top_block->emplace_back(new move_register {
                __dest, get_physical(10 + i)
            });
        } else { /* Excessive arguments in stack. */
            top_block->emplace_back(new load_memory {
                load_memory::WORD,get_virtual(),
                address_type {stack_address {top_asm,ssize_t(7 - i)}}
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
    for(auto __p : __func->stmt) visitBlock(__p);
}


void ASMbuilder::visitInit(IR::initialization *init) {
    runtime_assert("Not implemented!");
}


void ASMbuilder::visitCompare(IR::compare_stmt *__stmt) {
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
        return top_block->emplace_back(new load_immediate {
            get_virtual(__stmt->dest),__ans
        });
    }

    virtual_register *__lval = get_virtual(__stmt->lvar);
    virtual_register *__dest = get_virtual(__stmt->dest);

    if (__rhs) {
        if (__rhs->value == 0 && __stmt->op == __stmt->ADD)
            return top_block->emplace_back(new move_register {__dest,__lval});

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

    virtual_register *__rval = get_virtual(__stmt->rvar);

    return top_block->emplace_back(new arith_register {
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
    top_block->emplace_back(new branch_expression {
        get_virtual(__stmt->cond),
        get_edge(top_stmt,__stmt->br[0]),
        get_edge(top_stmt,__stmt->br[1])
    });
}

void ASMbuilder::visitCall(IR::call_stmt *__stmt) {

}

void ASMbuilder::visitLoad(IR::load_stmt *__stmt) {

}

void ASMbuilder::visitStore(IR::store_stmt *__stmt) {

}

void ASMbuilder::visitReturn(IR::return_stmt *__stmt) {
    virtual_register *__vir = get_virtual(__stmt->rval);
    top_block->emplace_back(new ret_expression {__vir});
}

void ASMbuilder::visitAlloc(IR::allocate_stmt *__stmt) {}

void ASMbuilder::visitGet(IR::get_stmt *__stmt) {
    size_t __unit = (--__stmt->dst->type).size();
    if (auto __idx = dynamic_cast <IR::integer_constant *> (__stmt->idx)) {
        size_t __offset = __idx->value * __unit;
        if (__stmt->mem) __offset += __stmt->mem * IR::MxPTRSIZE;
        offset_map[__stmt->dst] = __offset;
        return;
    }

    if ((__unit != 4 && __unit != 1) ||  __stmt->mem != __stmt->NPOS)
        runtime_assert("Not supported!");

    auto *__idx = get_virtual(__stmt->idx);
    auto *__src = get_virtual(__stmt->src);
    auto *__dst = get_virtual(__stmt->dst);

    if (__unit == 4) {
        auto *__vir = get_virtual();
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

}

void ASMbuilder::visitUnreachable(IR::unreachable_stmt *__stmt) {
    return top_block->emplace_back(new ret_expression {nullptr});
}


}