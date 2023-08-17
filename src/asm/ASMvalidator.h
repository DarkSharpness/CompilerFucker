#pragma once
#include "ASMnode.h"


namespace dark::ASM {


/* Help to convert invalid immediate into valid command. */
struct ASMvalidator : ASMvisitorbase {

    std::vector <block *> block_list;
    std::vector <node  *> node_list;

    /* Reference count of a register. */
    std::map <IR::temporary  *, virtual_register> temp_map;

    function *top;
    virtual_register *create_virtual() {
        return new virtual_register(top->vir_size++);
    }

    ASMvalidator(global_information &__info) {
        for(auto __p : __info.function_list)
            visitFunction(__p);
        __info.print(std::cout);
    }

    /* Load an immediate. */
    void force_load(value_type *&__ptr,immediate *__imm) {
        auto *__reg = create_virtual();
        __ptr = __reg;
        node_list.push_back(new load_immediate(__reg,__imm));
    }

    void visitFunction(function *);
    void visitBlock(block *);
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
