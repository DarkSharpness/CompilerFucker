#pragma once

#include "IRbase.h"
#include "IRnode.h"
#include "ASMnode.h"


namespace dark::ASM {


struct ASMbuilder : IR::IRvisitorbase {

    IR::function   *top_func;  /* Top IR  function. */
    function       *top_asm;   /* Top ASM function. */
    IR::block_stmt *top_stmt;  /* Top IR  block statement. */
    block          *top_block; /* Top ASM block statement. */

    std::map <IR::function   *, function> func_map;
    std::map <IR::block_stmt *,   block > block_map;
    std::map <IR::temporary  *, virtual_register> temp_map;

    inline static immediate    *__bool_true__ = create_immediate(0x01);
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

    function *get_function(IR::function *__func) {
        auto *__ans = &func_map[__func];
        if(__ans->name.empty()) __ans->name = __func->name;
        return __ans;
    }

    value_type *get_literal(IR::literal *__lit) {
        auto __name = __lit->data();
        switch(__name[0]) {
            case 't': return __bool_true__;
            case 'f':
            case 'n':
            case '0':
                return __zero__;
            case '@': /* Constant pointer is a symbol! */
                return new symbol {
                    std::string_view {__name.c_str() + 1, __name.size() - 1}
                };
        }
        return create_immediate(safe_cast <IR::integer_constant *> (__lit)->value);
    }

    value_type *get_variable(IR::variable *__var) {
        if(__var->name[0] == '@') {
            return new symbol {
                std::string_view {__var->name.c_str() + 1, __var->name.size() - 1}
            };
        }
        throw dark::error("Local variable cannot get directly!");
    }

    /* Get the virtual register for the temporary. */
    register_ *get_temporary(IR::temporary *__tmp) {
        auto [__iter,__flag] = temp_map.insert({__tmp, virtual_register {0}});
        if(__flag) __iter->second.index = top_asm->virtual_size++;
        return &__iter->second;
    }

    /**
     * @brief Get the value of a definition. \n
     * If global variable, it will return a symbol. \n
     * If local variable, it should fail (throw dark::error). \n
     * If temporary, it will return a virtual register. \n
     * If literal(except pointer literal), it will return an immediate/zero.
     * 
    */
    value_type *get_definition(IR::definition *__def) {
        if(auto __lit = dynamic_cast <IR::literal *> (__def))
            return get_literal(__lit);
        if(auto __tmp = dynamic_cast <IR::temporary *> (__def))
            return get_temporary(__tmp);
        if(auto __var = dynamic_cast <IR::variable *> (__def))
            return get_variable(__var);
        throw dark::error("Fxxk......this shouldn't happen!");
    }

    /* Load store only. */
    address_type *get_address(IR::non_literal *__tmp) {
        /* Global variable. */
        if(auto *__var = dynamic_cast <IR::variable *> (__tmp)) {
            if(__tmp->name[0] == '@') {
                return new global_address {
                    std::string_view {__tmp->name.c_str() + 1, __tmp->name.size() - 1}
                };
            } else {
                return new stack_address { top_asm,__var };
            }
        } else { /* Temporary. */
            return new register_address {
                get_temporary(safe_cast <IR::temporary *> (__tmp)),
                create_immediate(0)
            };
        }
    }

    /* Get the stack argument position. */
    address_type *get_stack_arg(size_t __bias) {
        return new register_address {
            physical_register::get_register(register_type::sp),
            create_immediate (__bias * 4)
        };
    }

    /* Check the phi expression and update phi expression if success.. */
    void check_phi(IR::phi_stmt *__phi) {
        if(!__phi) return;
        for(auto [__val,__label] : __phi->cond) {
            if(__label != top_stmt) continue;
                create_assign(get_temporary(__phi->dest), __val);
        }
    }

    /* Save a value into target register. */
    void create_assign(register_ *__reg,IR::definition *__def) {
        auto *__val = get_definition(__def);
        if(auto *__sym = dynamic_cast <symbol *> (__val)) {
            top_block->emplace_new(
                new load_symbol { __reg,__sym }
            );
        } else if(auto *__imm = dynamic_cast <immediate *> (__val)) {
            top_block->emplace_new(
                new load_immediate { __reg,__imm }
            );
        } else {
            top_block->emplace_new(
                new move_expr { __reg,safe_cast <register_ *> (__val) }
            );
        }
    }

    /* Load a value from target memory address. */
    void create_load(address_type *__add,IR::temporary *__def) {
        auto *__dst  = get_temporary(__def);
        auto *__load = new load_memory {
            __def->get_value_type().size() == 1 ? load_memory::BYTE : load_memory::WORD,
            __add,__dst
        };
        top_block->emplace_new(__load);
    }

    /* Save a value into target memory address. */
    void create_store(address_type *__add,IR::definition *__def) {
        auto *__val = get_definition(__def);
        register_ *__reg = nullptr;
        if (auto *__sym = dynamic_cast <symbol *> (__val)) {
            __reg = new virtual_register {top_asm->virtual_size++};
            top_block->emplace_new(
                new load_symbol { __reg,__sym }
            );
        } else if(auto *__imm = dynamic_cast <immediate *> (__val)) {
            __reg = new virtual_register {top_asm->virtual_size++};
            top_block->emplace_new(
                new load_immediate { __reg,__imm }
            );
        } else {
            __reg = safe_cast <register_ *> (__val);
        }

        auto *__store = new store_memory {
            __def->get_value_type().size() == 1 ? store_memory::BYTE : store_memory::WORD,
            __reg , __add
        };
        top_block->emplace_new(__store);
    }

    void create_jump(block *__dst) {
        top_block->emplace_new(new jump_expr { __dst } );
    }

    ~ASMbuilder() = default;
};





}