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
    inline static physical_register *__zero__ = get_register(register_type::zero);

    global_information global_info;

    ASMbuilder(std::vector <IR::initialization> &global_variables,
               std::vector <IR::function   >    &global_functions) {
        for(auto &&__var : global_variables)
            visitInit(&__var);
        for(auto &&__func : global_functions)
            visitFunction(&__func);

        for(auto &&__func : global_functions)
            global_info.function_list.push_back(&func_map.at(&__func));

    }

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
        return &block_map.try_emplace(__block,__block->label).first->second;
    }

    /* No renaming is done. */
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
                return new symbol {std::move(__name)};
            case 'c': /* String literals. */
                throw error("This shouldn't happen!");
        }
        return create_immediate(safe_cast <IR::integer_constant *> (__lit)->value);
    }

    value_type *get_variable(IR::variable *__var) {
        if(dynamic_cast <IR::global_variable *> (__var)) {
            return new symbol {__var->name};
        } else { /* Local variable / function argument. */
            auto __n = top_func->get_arg_index(__var);
            if (__n == size_t(-1)) throw dark::error("Local variable cannot get directly!");
            /* Function argument case. */
            if (__n < 8) {
                return get_register (
                    static_cast <register_type> (__n + 10)
                );
            } else {
                auto *__reg = new virtual_register {top_asm->vir_size++};
                top_block->emplace_new(new load_memory {
                    load_memory::WORD,
                    new stack_address { top_asm,__var },
                    __reg
                });
                return __reg;
            }
        }
    }

    /* Get the virtual register for the temporary. */
    register_ *get_virtual(IR::temporary *__tmp) {
        auto [__iter,__flag] = temp_map.insert({__tmp, virtual_register {0}});
        if(__flag) __iter->second.index = top_asm->vir_size++;
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
    value_type *get_value(IR::definition *__def) {
        if(auto __lit = dynamic_cast <IR::literal *> (__def))
            return get_literal(__lit);
        if(auto __tmp = dynamic_cast <IR::temporary *> (__def))
            return get_virtual(__tmp);
        if(auto __var = dynamic_cast <IR::variable *> (__def))
            return get_variable(__var);
        throw dark::error("Fxxk......this shouldn't happen!");
    }

    /* Load store only. */
    address_type *get_address(IR::non_literal *__tmp) {
        /* Global variable. */
        if(auto *__var = dynamic_cast <IR::variable *> (__tmp)) {
            if(dynamic_cast <IR::global_variable *> (__var)) {
                return new global_address {
                    std::string_view {__tmp->name}
                };
            } else {
                return new stack_address { top_asm,__var };
            }
        } else { /* Temporary. */
            return new register_address {
                get_virtual(safe_cast <IR::temporary *> (__tmp)),
                create_immediate(0)
            };
        }
    }

    /* Get the stack argument position. */
    address_type *get_stack_arg(size_t __bias) {
        return new register_address {
            get_register(register_type::sp),
            create_immediate (__bias * 4)
        };
    }

    /* Check the phi expression and update phi expression if success.. */
    void check_phi(IR::phi_stmt *__phi) {
        if(!__phi) return;
        for(auto [__val,__label] : __phi->cond) {
            if(__label != top_stmt) continue;
                create_assign(get_virtual(__phi->dest), __val);
        }
    }

    /* Save a value into target register. */
    void create_assign(register_ *__reg,IR::definition *__def) {
        auto *__val = get_value(__def);
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
        top_block->emplace_new(new load_memory {
            __def->get_value_type().size() == 1 ? 
                load_memory::BYTE : load_memory::WORD,
            __add, get_virtual(__def)
        });
    }

    /* Save a value into target memory address. */
    void create_store(address_type *__add,IR::definition *__def) {
        auto *__val = get_value(__def);
        register_ *__reg = nullptr;
        if (auto *__sym = dynamic_cast <symbol *> (__val)) {
            __reg = new virtual_register {top_asm->vir_size++};
            top_block->emplace_new(
                new load_symbol { __reg,__sym }
            );
        } else if(auto *__imm = dynamic_cast <immediate *> (__val)) {
            __reg = new virtual_register {top_asm->vir_size++};
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