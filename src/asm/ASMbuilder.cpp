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
    for(auto __p : ctx->stmt) visitBlock(__p);
}

void ASMbuilder::visitInit(IR::initialization *ctx) {
}

void ASMbuilder::visitCompare(IR::compare_stmt *ctx) {
    
}


void ASMbuilder::visitBinary(IR::binary_stmt *ctx) {
    

}


void ASMbuilder::visitJump(IR::jump_stmt *ctx) {
    /* If phi block, modify the value beforehand. */
    check_phi(ctx->dest->is_phi_block());
    create_jump(get_block(ctx->dest));
}

void ASMbuilder::visitBranch(IR::branch_stmt *ctx) {
    auto *__cond    = get_definition(ctx->cond);
    auto *__true    = get_block(ctx->br[0]);
    auto *__false   = get_block(ctx->br[1]);
    auto *__branch  = new bool_expr {__cond, __true};

    if(auto *__phi  = ctx->br[0]->is_phi_block()) {
        block *__tmp = new block {{.name = __true->name + ".qwq"}};
        std::swap(top_block,__tmp);
        check_phi(__phi);
        create_jump(__true);
        std::swap(top_block,__tmp);
        __branch->dest = __tmp;
    }

    top_block->emplace_new(__branch);
    check_phi(ctx->br[1]->is_phi_block());
    create_jump(__false);
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
    auto *__var = get_definition(ctx->src);
    size_t __n  = 0;

    auto *__tmp = get_temporary(ctx->dst);
    if(auto __idx = dynamic_cast <IR::integer_constant *> (ctx->idx))
        __n = __idx->value * __type.size();
    else {
        auto *__ind = get_definition(ctx->idx);
        if(__type.size() == 1) {
            auto *__get = new arith_expr {
                arith_expr::ADD,__var,__ind,__tmp
            };
            top_block->emplace_new(__get);
        } else { /* size == 4 */
            auto *__sll = new arith_expr {
                arith_expr::SLL,__ind,new immediate {2},__tmp
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
        arith_expr::ADD,__var,new immediate {__n},__tmp
    };
    top_block->emplace_new(__get);

}


/* Phi has been done in previous visit. */
void ASMbuilder::visitPhi(IR::phi_stmt *ctx) { return; }

void ASMbuilder::visitReturn(IR::return_stmt *ctx) {
    if(ctx->rval) create_store(get_stack_arg(0),ctx->rval);
    top_block->emplace_new(new return_expr {top_asm});
}

void ASMbuilder::visitUnreachable(IR::unreachable_stmt *ctx) {
    top_block->emplace_new(new return_expr {top_asm});
}



}