#pragma once
#include "ASMnode.h"


namespace dark::ASM {


/* Help to convert registers into physical ones. */
struct ASMallocator : ASMvisitorbase {

    void visitArithExpr(arith_expr *) override;
    void visitBranchExpr(branch_expr *) override;
    void visitSltExpr(slt_expr *) override;
    void visitEqExpr(eq_expr *) override;
    void visitNotExpr(not_expr *) override;
    void visitBoolExpr(bool_expr *) override;
    void visitLoadMemory(load_memory *) override;
    void visitStoreMemory(store_memory *) override;
    void visitLoadSymbol(load_symbol *) override;
    void visitLoadImmediate(load_immediate *) override;
    void visitCallExpr(call_expr *) override;
    void visitMoveExpr(move_expr *) override;
    void visitJumpExpr(jump_expr *) override;
    void visitReturnExpr(return_expr *) override;
};


}
