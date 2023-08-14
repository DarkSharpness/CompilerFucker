#pragma once

#include "IRbase.h"
#include "IRnode.h"
#include "ASMnode.h"


namespace dark::ASM {


struct ASMbuilder : IR::IRvisitorbase {

    IR::function   *top_func;  /* Top IR  function. */
    IR::block_stmt *top_stmt;  /* Top IR  block statement. */
    block          *top_block; /* Top ASM block statement. */
    std::map <IR::block_stmt *, block> block_map;

    inline static low_immediate __bool_true__ = low_immediate(0xff);
    inline static physical_register *__zero__ = physical_register::get_register(register_type::zero);

    void visitBlock(IR::block_stmt*) override;
    void visitFunction(IR::function*) override;
    void visitInit(IR::initialization*) override;

    void visitCompare(IR::compare_stmt *) override;
    void visitBinary(IR::binary_stmt *) override;
    void visitJump(IR::jump_stmt *) override;
    void visitBranch(IR::branch_stmt *) override;
    void visitCall(IR::call_stmt *) override;
    void visitLoad(IR::load_stmt *) override;
    void visitStore(IR::store_stmt *) override;
    void visitReturn(IR::return_stmt *) override;
    void visitAlloc(IR::allocate_stmt *) override;
    void visitGet(IR::get_stmt *) override;
    void visitPhi(IR::phi_stmt *) override;
    void visitUnreachable(IR::unreachable_stmt *) override;

    block *get_block(IR::block_stmt *__block) {
        auto *__ans = &block_map[__block];
        if(__ans->name.empty()) __ans->name = __block->label;
        return __ans;
    }

    value_type *get_register(IR::definition *__reg) {
        if(auto *__lit = dynamic_cast <IR::literal *> (__reg)) {
            auto __name = __lit->data();
            if(__name == "true") {
                return &__bool_true__;
            } else if(__name == "false" || __name == "null") {
                return __zero__;
            }
            /* Integer constant case. */
            auto __int = safe_cast <IR::integer_constant *> (__lit);
            if(is_low_immediate(__int->value))
                return new low_immediate(__int->value);
            else { /* High immediate. */
                auto *__virtual_reg = new virtual_register;
                top_block->emplace_new(
                    new ASM::load_immediate(
                        __virtual_reg,
                        new full_immediate(__int->value)
                    )
                );
                return __virtual_reg;
            }
        } else if(auto *__var = dynamic_cast <IR::variable *> (__reg)) {
            if(__var->name[0] == '@') { /* Global. */
                auto *__virtual_reg = new virtual_register;
                top_block->emplace_new(
                    new ASM::load_address {
                        {__var->name.data() + 1, __var->name.size() - 1},
                        __virtual_reg,
                    }
                );
                return __virtual_reg;
            } else {
                auto *__virtual_reg = new virtual_register;
                top_block->emplace_new(
                    new load_expr {
                        load_expr::WORD, /* Aligned to 4 bytes */
                        new stack_immediate {
                            top_func,
                            __var
                        },
                        physical_register::get_register(register_type::sp),
                        __virtual_reg
                    }
                );             
            }
        }
    }

    ~ASMbuilder() = default;
};





}