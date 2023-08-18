#include "ASMvalidator.h"


namespace dark::ASM {

void ASMvalidator::visitFunction(function *ctx) {
    top = ctx;
    block_list.clear();
    for(auto __stmt : ctx->stmt) visitBlock(__stmt);
    ctx->stmt = block_list;
}

void ASMvalidator::visitBlock(block *ctx) {
    node_list.clear();
    for(auto __expr : ctx->expr) visit(__expr);
    ctx->expr = node_list;
    block_list.push_back(ctx);
}

/* Todo: optimize all useless arith_expr out. */
void ASMvalidator::visitArithExpr(arith_expr *ctx) {
    auto __limm = dynamic_cast <immediate *> (ctx->lval);
    auto __rimm = dynamic_cast <immediate *> (ctx->rval);
    node *__tmp = nullptr;
    if(__limm && __rimm)
        throw error("This should be avoided in constant folding!");
    switch(ctx->op) {
        case arith_expr::AND:
        case arith_expr::OR_:
        case arith_expr::XOR:
        case arith_expr::ADD:
            if(__limm) std::swap(ctx->lval, ctx->rval);
            break;

        case arith_expr::SUB:
            if(__rimm) {
                /**
                 * sub rd,rs,imm -->
                 * 
                 * add rd,rs,-imm
                */
                ctx->op   = arith_expr::ADD;
                ctx->rval = create_immediate(-__rimm->value);
            }
            else if(__limm) {
                /**
                 * sub rd,imm,rs -->
                 * 
                 * sub rd,zero,rs
                 * add rd,rd,imm
                */
                ctx->lval = get_register(register_type::zero);
                __tmp = new arith_expr {
                    arith_expr::ADD,
                    ctx->dest,
                    __limm,
                    ctx->dest
                };
            }
            break;

        /* No immediate is supported. */
        case arith_expr::MUL:
        case arith_expr::DIV:
        case arith_expr::REM:
            if(__rimm) /* Todo: optimize it to bit operation if possible. */
                force_load(ctx->rval,__rimm);

        /* Only right immediate is supported. */
        case arith_expr::SLL:
        case arith_expr::SRA:
            if(__limm)
                force_load(ctx->lval,__limm);
    }

    node_list.push_back(ctx);
    if(__tmp) node_list.push_back(__tmp);
}

void ASMvalidator::visitBranchExpr(branch_expr *ctx) {
    throw error("This will only be used in optimization!");
    // node_list.push_back(ctx);
}

void ASMvalidator::visitSltExpr(slt_expr *ctx) {
    auto *__limm = dynamic_cast <immediate *> (ctx->lval);
    auto *__rimm = dynamic_cast <immediate *> (ctx->rval);
    if(__limm) throw error("This should be avoided in ASM building!");
    node_list.push_back(ctx);
}

void ASMvalidator::visitEqExpr(eq_expr *ctx) {
    /* Do nothing. */
    node_list.push_back(ctx);
}

void ASMvalidator::visitNotExpr(not_expr *ctx) {
    /* Do nothing. */
    node_list.push_back(ctx);
}

void ASMvalidator::visitBoolExpr(bool_expr *ctx) {
    /* Do nothing. */
    node_list.push_back(ctx);
}

void ASMvalidator::visitLoadMemory(load_memory *ctx) {
    /* Do nothing. */
    node_list.push_back(ctx);
}

void ASMvalidator::visitStoreMemory(store_memory *ctx) {
    auto *__add = dynamic_cast <global_address *> (ctx->addr);
    if(__add) ctx->temp = create_virtual();
    node_list.push_back(ctx);
}

void ASMvalidator::visitLoadSymbol(load_symbol *ctx) {
    /* Do nothing. */
    node_list.push_back(ctx);
}
void ASMvalidator::visitLoadImmediate(load_immediate *ctx) {
    /* Do nothing. */
    node_list.push_back(ctx);
}
void ASMvalidator::visitCallExpr(call_expr *ctx) {
    /* Do nothing. */
    node_list.push_back(ctx);
}
void ASMvalidator::visitMoveExpr(move_expr *ctx) {
    /* Do nothing. */
    node_list.push_back(ctx);
}
void ASMvalidator::visitJumpExpr(jump_expr *ctx) {
    /* Do nothing. */
    node_list.push_back(ctx);
}
void ASMvalidator::visitReturnExpr(return_expr *ctx) {
    /* Do nothing. */
    node_list.push_back(ctx);
}

}