#include "ASMbuilder.h"


namespace dark::ASM {

/* It will return a block pointer. */
void ASMbuilder::visitBlock(IR::block_stmt *ctx) {
    top_stmt  = ctx;
    top_block = get_block(ctx);
    for(auto __p : ctx->stmt) visit(__p);
}

void ASMbuilder::visitFunction(IR::function *ctx) {}
void ASMbuilder::visitInit(IR::initialization *ctx) {}

void ASMbuilder::visitCompare(IR::compare_stmt  *ctx) {}
void ASMbuilder::visitBinary(IR::binary_stmt *ctx) {}


void ASMbuilder::visitJump(IR::jump_stmt *ctx) {
    /* If phi block, modify the value beforehand. */
    auto *__dest = get_block(ctx->dest);
    if(auto __phi = ctx->dest->is_phi_block()) {
        for(auto [__val,__label] : __phi->cond) {
            if(__label != top_stmt) continue;

            break;
        }
    }
    /* Jump !!! */
    auto *__jump = new jump_expr {__dest};
    top_block->expr.push_back(__jump);
}

void ASMbuilder::visitBranch(IR::branch_stmt  *ctx) {}
void ASMbuilder::visitCall(IR::call_stmt  *ctx) {}
void ASMbuilder::visitLoad(IR::load_stmt  *ctx) {}
void ASMbuilder::visitStore(IR::store_stmt  *ctx) {}
void ASMbuilder::visitReturn(IR::return_stmt  *ctx) {}
void ASMbuilder::visitAlloc(IR::allocate_stmt  *ctx) {}
void ASMbuilder::visitGet(IR::get_stmt  *ctx) {}
void ASMbuilder::visitPhi(IR::phi_stmt  *ctx) {}
void ASMbuilder::visitUnreachable(IR::unreachable_stmt  *ctx) {}



}