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
    for(auto __expr : ctx->expr) visit(__expr);
    ctx->expr = node_list;
    block_list.push_back(ctx);
}

void ASMvisitor::visitArithExpr(arith_expr *ctx) {
    update_rvalue(ctx->lval,ctx->rval);
    update_lvalue(ctx->dest);
    node_list.push_back(ctx);
}
void ASMvisitor::visitBranchExpr(branch_expr *ctx) {
    throw error("This will only be used in optimization!");
}

void ASMvisitor::visitSltExpr(slt_expr *ctx) {
    update_rvalue(ctx->lval,ctx->rval);
    update_lvalue(ctx->dest);
    node_list.push_back(ctx);
}

void ASMvisitor::visitEqExpr(eq_expr *ctx) {
    update_rvalue(ctx->rval);
    update_lvalue(ctx->dest);
    node_list.push_back(ctx);
}
void ASMvisitor::visitNotExpr(not_expr *ctx) {
    update_rvalue(ctx->rval);
    update_lvalue(ctx->dest);
    node_list.push_back(ctx);
}
void ASMvisitor::visitBoolExpr(bool_expr *ctx) {
    update_rvalue(ctx->cond);
    node_list.push_back(ctx);
}
void ASMvisitor::visitLoadMemory(load_memory *ctx) {
    update_lvalue(ctx->dest);
    if(auto *__addr = dynamic_cast <register_address *> (ctx->addr)) {
        value_type *__val = __addr->reg;
        update_rvalue(__val);
        __addr->reg = safe_cast <physical_register *> (__val);
    }

    node_list.push_back(ctx);
}
void ASMvisitor::visitStoreMemory(store_memory *ctx) {
    /* Special pointer! */
    update_lvalue(ctx->temp);
    {
        value_type *__val = ctx->from;
        update_rvalue(__val);
        ctx->from = safe_cast <physical_register *> (__val);
    }
    if(auto *__addr = dynamic_cast <register_address *> (ctx->addr)) {
        value_type *__val = __addr->reg;
        update_rvalue(__val);
        __addr->reg = safe_cast <physical_register *> (__val);
    }

    node_list.push_back(ctx);
}

void ASMvisitor::visitLoadSymbol(load_symbol *ctx) {
    update_lvalue(ctx->dst);
    node_list.push_back(ctx);
}

void ASMvisitor::visitLoadImmediate(load_immediate *ctx) {
    update_lvalue(ctx->dst);
    node_list.push_back(ctx);
}

void ASMvisitor::visitCallExpr(call_expr *ctx) {
    node_list.push_back(ctx);
}

void ASMvisitor::visitMoveExpr(move_expr *ctx) {
    {
        value_type *__val = ctx->src;
        update_rvalue(__val);
        ctx->src = safe_cast <physical_register *> (__val);
    }
    update_lvalue(ctx->dst);
    node_list.push_back(ctx);
}
void ASMvisitor::visitJumpExpr(jump_expr *ctx) {
    node_list.push_back(ctx);
}
void ASMvisitor::visitReturnExpr(return_expr *ctx) {
    node_list.push_back(ctx);
}


}