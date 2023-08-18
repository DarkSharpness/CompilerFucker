#include "ASMvisitor.h"


namespace dark::ASM {

void ASMvisitor::visitFunction(function *) {

}
void ASMvisitor::visitBlock(block *) {

}

void ASMvisitor::visitArithExpr(arith_expr *) {}
void ASMvisitor::visitBranchExpr(branch_expr *) {}
void ASMvisitor::visitSltExpr(slt_expr *) {}
void ASMvisitor::visitEqExpr(eq_expr *) {}
void ASMvisitor::visitNotExpr(not_expr *) {}
void ASMvisitor::visitBoolExpr(bool_expr *) {}
void ASMvisitor::visitLoadMemory(load_memory *) {}
void ASMvisitor::visitStoreMemory(store_memory *) {}
void ASMvisitor::visitLoadSymbol(load_symbol *) {}
void ASMvisitor::visitLoadImmediate(load_immediate *) {}
void ASMvisitor::visitCallExpr(call_expr *) {}
void ASMvisitor::visitMoveExpr(move_expr *) {}
void ASMvisitor::visitJumpExpr(jump_expr *) {}
void ASMvisitor::visitReturnExpr(return_expr *) {}


}