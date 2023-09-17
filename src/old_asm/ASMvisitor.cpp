#include "ASMvisitor.h"


namespace dark::ASM {

void ASMvisitor::visitFunction(function *ctx) {
    top = ctx;
    block_list.clear();
    for(auto __stmt : ctx->stmt) visitBlock(__stmt);
    ctx->stmt = block_list;
}
void ASMvisitor::visitBlock(block *ctx) {
    node_list.clear();
    for(auto __expr : ctx->expr)
        visit(__expr);
    ctx->expr = node_list;
    block_list.push_back(ctx);
}

void ASMvisitor::visitArithExpr(arith_expr *ctx) {
    update_rvalue(ctx->lval);
    update_rvalue(ctx->rval);
    node_list.push_back(ctx);
    update_lvalue(ctx->dest);
}
void ASMvisitor::visitBranchExpr(branch_expr *ctx) {
    throw error("This will only be used in optimization!");
}

void ASMvisitor::visitSltExpr(slt_expr *ctx) {
    update_rvalue(ctx->lval);
    update_rvalue(ctx->rval);
    node_list.push_back(ctx);
    update_lvalue(ctx->dest);
}

void ASMvisitor::visitEqExpr(eq_expr *ctx) {
    update_rvalue(ctx->rval);
    node_list.push_back(ctx);
    update_lvalue(ctx->dest);
}

void ASMvisitor::visitNotExpr(not_expr *ctx) {
    update_rvalue(ctx->rval);
    node_list.push_back(ctx);
    update_lvalue(ctx->dest);
}
void ASMvisitor::visitBoolExpr(bool_expr *ctx) {
    update_rvalue(ctx->cond);
    node_list.push_back(ctx);
}
void ASMvisitor::visitLoadMemory(load_memory *ctx) {
    if(auto *__addr = dynamic_cast <register_address *> (ctx->addr))
        update_rvalue(__addr->reg);
    node_list.push_back(ctx);
    update_lvalue(ctx->dest);
}
void ASMvisitor::visitStoreMemory(store_memory *ctx) {
    /* Special pointer! */
    if(update_lvalue(ctx->temp)) node_list.pop_back();
    update_rvalue(ctx->from);
    if(auto *__addr = dynamic_cast <register_address *> (ctx->addr))
        update_rvalue( __addr->reg);
    node_list.push_back(ctx);
}

void ASMvisitor::visitLoadSymbol(load_symbol *ctx) {
    node_list.push_back(ctx);
    update_lvalue(ctx->dst);
}

void ASMvisitor::visitLoadImmediate(load_immediate *ctx) {
    node_list.push_back(ctx);
    update_lvalue(ctx->dst);
}

void ASMvisitor::visitCallExpr(call_expr *ctx) {
    /* No store should be done beforehand. */
    node_list.push_back(ctx);
}

void ASMvisitor::visitMoveExpr(move_expr *ctx) {
    update_rvalue(ctx->src);
    node_list.push_back(ctx);
    update_lvalue(ctx->dst);
}

void ASMvisitor::visitJumpExpr(jump_expr *ctx) {
    node_list.push_back(ctx);
}

void ASMvisitor::visitReturnExpr(return_expr *ctx) {
    node_list.push_back(ctx);
}


}