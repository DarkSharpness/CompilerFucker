#include "ASMbuilder.h"


namespace dark::ASM {

/* It will return a block pointer. */
void ASMbuilder::visitBlock(IR::block_stmt *ctx) {
    top_stmt  = ctx;
    top_block = get_block(ctx);
    top_asm->emplace_new(top_block);
    for(auto __p : ctx->stmt) visit(__p);
}

void ASMbuilder::visitFunction(IR::function *ctx) {
    top_func = ctx;
    top_asm  = get_function(ctx);
    top_asm->emplace_arg(ctx->args);
    for(auto __p : ctx->stmt) visitBlock(__p);
}

/* Visit the initialization. */
void ASMbuilder::visitInit(IR::initialization *ctx) {
    auto *__var = ctx->dest;
    auto *__lit = ctx->lite;
    if (auto __str = dynamic_cast <IR::string_constant *> (__lit))
        global_info.rodata_list.push_back(*ctx);
    else
        global_info.data_list.push_back(*ctx);
}

/* Visit the comparison. */
void ASMbuilder::visitCompare(IR::compare_stmt *ctx) {
    auto *__lhs = get_value(ctx->lvar);
    auto *__rhs = get_value(ctx->rvar);
    auto *__reg = get_virtual(ctx->dest);

    /* No need to unify for booleans. */
    if(ctx->lvar->get_value_type().size() == 1) {
        top_block->emplace_new(new arith_expr {
            arith_expr::XOR, /* xor first */
            get_value(ctx->lvar),
            get_value(ctx->rvar),
            __reg
        }); return;
    }

    auto *__limm = dynamic_cast <immediate *> (__lhs);
    auto *__rimm = dynamic_cast <immediate *> (__rhs);
    bool  __flag = false;
    if(__limm && __rimm) throw error("Cannot compare two constants!");

    switch(ctx->op) {
        case IR::compare_stmt::EQ:
        case IR::compare_stmt::NE:
            top_block->emplace_new(new arith_expr {
                arith_expr::XOR,__lhs,__rhs,__reg
            });
            if(ctx->op == IR::compare_stmt::NE)
                top_block->emplace_new(new eq_expr {__reg,__reg});
            else
                top_block->emplace_new(new not_expr {__reg,__reg});
            return;

        case IR::compare_stmt::GT: std::swap(__lhs,__rhs); std::swap(__limm,__rimm);
        case IR::compare_stmt::LT:
            if(!__limm) top_block->emplace_new(new slt_expr {__lhs,__rhs,__reg});
            else {
                top_block->emplace_new(new slt_expr {
                    __rhs,create_immediate(__limm->value + 1),__reg
                });
                __flag = true;
            }
            break;

        case IR::compare_stmt::LE: std::swap(__lhs,__rhs); std::swap(__limm,__rimm);
        case IR::compare_stmt::GE:
            if(!__limm) {
                top_block->emplace_new(new slt_expr {__lhs,__rhs,__reg});
                __flag = true;
            } else {
                top_block->emplace_new(new slt_expr {
                    __rhs,create_immediate(__limm->value + 1),__reg
                });
            }
    }
    if(__flag) top_block->emplace_new(new not_expr {__reg,__reg});
}


void ASMbuilder::visitBinary(IR::binary_stmt *ctx) {
    top_block->emplace_new(new arith_expr {
        static_cast <decltype(arith_expr::op)> (ctx->op),
        get_value(ctx->lvar),
        get_value(ctx->rvar),
        get_virtual (ctx->dest)
    });
}


void ASMbuilder::visitJump(IR::jump_stmt *ctx) {
    /* If phi block, modify the value beforehand. */
    check_phi(ctx->dest->is_phi_block());
    create_jump(get_block(ctx->dest));
}

void ASMbuilder::visitBranch(IR::branch_stmt *ctx) {
    auto *__cond    = get_value(ctx->cond);
    auto *__true    = get_block(ctx->br[0]);
    auto *__false   = get_block(ctx->br[1]);
    auto *__branch  = new bool_expr {__cond, __true};

    block *__tmp = nullptr;
    if(auto *__phi  = ctx->br[0]->is_phi_block()) {
        __tmp = new block {__true->name + ".qwq"};
        std::swap(top_block,__tmp);
        check_phi(__phi);
        create_jump(__true);
        std::swap(top_block,__tmp);
        __branch->dest = __tmp;
    }

    top_block->emplace_new(__branch);
    check_phi(ctx->br[1]->is_phi_block());
    create_jump(__false);
    if(__tmp) top_asm->emplace_new(__tmp);
}

void ASMbuilder::visitCall(IR::call_stmt *ctx) {
    for(size_t i = 0 ; i < ctx->args.size() ; i++) {
        auto *__p = ctx->args[i];
        if(i < 8) { /* Pass by registers. */
            create_assign(
                physical_register::get_register(
                    static_cast <register_type> (i + 10)
                ), __p
            );
        } else { /* Pass by stack data. */
            create_store(get_stack_arg(i - 8),__p);
        }
    }
    top_asm->update_size(ctx->func);
    top_block->emplace_new(new call_expr {get_function(ctx->func)});

    /* Need to store from a0 to target register! */
    if(ctx->func->type.name() != "void") {
        top_block->emplace_new(new move_expr {
            get_virtual(ctx->dest),
            physical_register::get_register(register_type::a0)
        });
    }
}

void ASMbuilder::visitLoad(IR::load_stmt *ctx) {
    create_load(get_address(ctx->src),ctx->dst);
}

void ASMbuilder::visitStore(IR::store_stmt *ctx) {
    create_store(get_address(ctx->dst), ctx->src);
}

void ASMbuilder::visitAlloc(IR::allocate_stmt *ctx) {
    top_asm->emplace_var(ctx->dest);
}


/* Return the element pointer */
void ASMbuilder::visitGet(IR::get_stmt *ctx) {
    auto __type = ctx->src->get_point_type();
    auto *__var = get_value(ctx->src);
    size_t __n  = 0;

    auto *__tmp = get_virtual(ctx->dst);
    if(auto __idx = dynamic_cast <IR::integer_constant *> (ctx->idx))
        __n = __idx->value * __type.size();
    else {
        auto *__ind = get_value(ctx->idx);
        if(__type.size() == 1) {
            auto *__get = new arith_expr {
                arith_expr::ADD,__var,__ind,__tmp
            };
            top_block->emplace_new(__get);
        } else { /* size == 4 */
            auto *__sll = new arith_expr {
                arith_expr::SLL,__ind,create_immediate(2),__tmp
            };
            top_block->emplace_new(__sll);
            auto *__get = new arith_expr {
                arith_expr::ADD,__var,__tmp,__tmp
            };
            top_block->emplace_new(__get);
        }
        __var = __tmp;
    }

    if(ctx->mem != ctx->NPOS) {
        auto *__class = safe_cast <const IR::class_type *> (__type.type);
        __n += __class->get_bias(ctx->mem);
    }

    auto *__get = new arith_expr {
        arith_expr::ADD,__var,create_immediate(__n),__tmp
    };
    top_block->emplace_new(__get);

}


/* Phi has been done in previous visit. */
void ASMbuilder::visitPhi(IR::phi_stmt *ctx) { return; }

void ASMbuilder::visitReturn(IR::return_stmt *ctx) {
    if(ctx->rval) create_assign(
        physical_register::get_register(register_type::a0),
        ctx->rval
    );
    top_block->emplace_new(new return_expr {top_asm});
}

void ASMbuilder::visitUnreachable(IR::unreachable_stmt *ctx) {
    /* Nothing is done. */
    top_block->emplace_new(new return_expr {top_asm});
}



}