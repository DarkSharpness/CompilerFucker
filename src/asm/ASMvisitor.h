#pragma once
#include "ASMnode.h"
#include "ASMallocator.h"


namespace dark::ASM {

/* Help to convert registers into physical ones. */
struct ASMvisitor : ASMvisitorbase {
    std::vector <block *> block_list;
    std::vector <node  *> node_list;

    function *top;

    /* Global information updator/ */
    ASMvisitor(global_information &__info) {
        for(auto __p : __info.function_list)
            visitFunction(__p);
        __info.print(std::cout);
    }

    void update_rvalue(value_type *&lval,value_type *&rval) {
        std::vector <virtual_register *> __list;
        __list.reserve(2);
        if(auto __reg = dynamic_cast <virtual_register *> (lval))
            __list.push_back(__reg);
        if(auto __reg = dynamic_cast <virtual_register *> (rval))
            __list.push_back(__reg);
        if(__list.empty()) return;

        auto __load = top->__alloc->access(std::move(__list));
        for(auto __p : __load) {
            if(__p.is_store()) {
                node_list.push_back(new store_memory {
                    store_memory::WORD,
                    __p.reg,
                    new temporary_address {top,__p.spos}
                });
            }
            if(__p.is_load()) {
                node_list.push_back(new load_memory {
                    load_memory::WORD,
                    new temporary_address {top,__p.lpos},
                    __p.reg
                });
            }
        }

        bool i = false;
        if(dynamic_cast <virtual_register *> (lval))
            lval = __load[0].reg , i = true;
        if(dynamic_cast <virtual_register *> (rval))
            rval = __load[i].reg;
    }

    void update_rvalue(value_type *&lval) {
        std::vector <virtual_register *> __list;
        if(auto __reg = dynamic_cast <virtual_register *> (lval))
            __list.push_back(__reg);
        if(__list.empty()) return;

        auto __p = top->__alloc->access(std::move(__list))[0];
        if(__p.is_store()) {
            node_list.push_back(new store_memory {
                store_memory::WORD,
                __p.reg,
                new temporary_address {top,__p.spos}
            });
        }
        if(__p.is_load()) {
            node_list.push_back(new load_memory {
                load_memory::WORD,
                new temporary_address {top,__p.lpos},
                __p.reg
            });
        }
        lval = __p.reg;
    }

    void update_lvalue(register_ *&__reg) {
        auto __vir = dynamic_cast <virtual_register *> (__reg);
        if(!__vir) return;
        auto __p = top->__alloc->allocate(__vir);
        if(__p.is_store()) {
            node_list.push_back(new store_memory {
                store_memory::WORD,
                __p.reg,
                new temporary_address {top,__p.pos}
            });
        }
        __reg = __p.reg;
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