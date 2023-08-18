#include "ASMcounter.h"


namespace dark::ASM {

void ASMcounter::visitFunction(function *ctx) {
    for(auto __stmt : ctx->stmt) {
        for(auto __node : __stmt->expr)
            visit(__node);
        auto *__alloc = new ASMallocator;
        // std::swap(__alloc->counter, counter);
        ctx ->__alloc = __alloc;
    }
}

void ASMcounter::visitArithExpr(arith_expr *ctx) {
    update(ctx->lval);
    update(ctx->rval);
    update(ctx->dest);
}
void ASMcounter::visitBranchExpr(branch_expr *ctx) {
    update(ctx->lval);
    update(ctx->rval);
}
void ASMcounter::visitSltExpr(slt_expr *ctx) {
    update(ctx->lval);
    update(ctx->rval);
    update(ctx->dest);
}
void ASMcounter::visitEqExpr(eq_expr *ctx) {
    update(ctx->rval);
    update(ctx->dest);
}
void ASMcounter::visitNotExpr(not_expr *ctx) {
    update(ctx->rval);
    update(ctx->dest);
}
void ASMcounter::visitBoolExpr(bool_expr *ctx) {
    update(ctx->cond);
}
void ASMcounter::visitLoadMemory(load_memory *ctx) {
    update(ctx->dest);
    if(auto *__addr = dynamic_cast <register_address *> (ctx->addr))
        update(__addr->reg);
}
void ASMcounter::visitStoreMemory(store_memory *ctx) {
    update(ctx->from);
    update(ctx->temp);
    if(auto *__addr = dynamic_cast <register_address *> (ctx->addr))
        update(__addr->reg);
}
void ASMcounter::visitLoadSymbol(load_symbol *ctx) {
    update(ctx->dst);
}
void ASMcounter::visitLoadImmediate(load_immediate *ctx) {
    update(ctx->dst);
}
void ASMcounter::visitCallExpr(call_expr *ctx) {}
void ASMcounter::visitMoveExpr(move_expr *ctx) {
    update(ctx->dst);
    update(ctx->src);
}

void ASMcounter::visitJumpExpr(jump_expr *ctx) {}
void ASMcounter::visitReturnExpr(return_expr *ctx) {}

}