#pragma once

#include "ASMnode.h"
#include "ASMallocator.h"


namespace dark::ASM {

struct ASMcounter : ASMvisitorbase {
    std::map <virtual_register *, size_t> counter;

    /* Global information updator. */
    ASMcounter(global_information &__info) {
        for(auto __p : __info.function_list)
            visitFunction(__p);
    }

    void update(value_type *__val) {
        auto *__reg = dynamic_cast <virtual_register *> (__val);
        if(__reg != nullptr) ++counter[__reg];
    }

    void visitFunction(function *);
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
    ~ASMcounter() = default;
};


}