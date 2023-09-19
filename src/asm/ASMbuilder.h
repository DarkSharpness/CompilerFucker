#pragma once

#include "IRbase.h"
#include "ASMnode.h"

#include <unordered_map>

namespace dark::ASM {


struct ASMbuilder final : IR::IRvisitorbase {
    IR::function   *top_func;  /* Top IR  function. */
    function       *top_asm;   /* Top ASM function. */
    IR::block_stmt *top_stmt;  /* Top IR  block statement. */
    block          *top_block; /* Top ASM block statement. */

    std::unordered_map <IR::function   *, function> func_map;
    std::unordered_map <IR::block_stmt *,   block > block_map;
    std::unordered_map <IR::non_literal  *, virtual_register *> temp_map;
    std::unordered_map <IR::temporary *,size_t> offset_map;

    struct branch_node {
        IR::block_stmt *key[2] = {};
        block          *val[2] = {};
    };

    std::unordered_map <IR::block_stmt *,branch_node> branch_map;

    global_information global_info;

    ASMbuilder(std::vector <IR::initialization> &global_variables,
               std::vector <IR::function>       &global_functions) {
        for(auto &&__var : global_variables) visitInit(&__var);

        for(auto &&__func : global_functions) visitFunction(&__func);

        for(auto &&__func : global_functions)
            global_info.function_list.push_back(&func_map.at(&__func));
    }

    void visitBlock(IR::block_stmt *) override;
    void visitFunction(IR::function *) override;
    void visitInit(IR::initialization *) override;

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

    address_type get_address(IR::definition *__def) {
        if (auto __global = dynamic_cast <IR::global_variable *> (__def)) {
            auto *__vir = get_virtual();
            top_block->emplace_back(new load_high{__vir,__global});

        }
    }

    template <size_t __off>
    block *get_virtual_block(IR::block_stmt *__old) {
        static_assert(__off < 10 && __off > 0);
        size_t __hash = std::hash <IR::block_stmt *> () (__old) + __off;
        auto *__block = reinterpret_cast <IR::block_stmt *> (__hash);
        auto  __name  = string_join(__old->label,"_v_",char(__off + '0'));
        return &block_map.try_emplace(__block,__name).first->second;
    }


    block *get_block(IR::block_stmt *__block) {
        return &block_map.try_emplace(__block,__block->label).first->second;
    }

    /* No renaming is done. */
    function *get_function(IR::function *__func) {
        auto *__ans = &func_map[__func];
        if(__ans->name.empty()) {
            __ans->name = __func->name;
            __ans->func_ptr = __func;
        }
        return __ans;
    }

    /**
     * @brief Return the virtual block of a phi.
     */
    block *get_edge(IR::block_stmt *prev,IR::block_stmt *next) {
        if (!dynamic_cast <IR::phi_stmt *> (next->stmt.front()))
            return get_block(next);            

        auto &__ref = branch_map[prev];

        if (__ref.key[0] == next) return __ref.val[0];
        if (__ref.key[1] == next) return __ref.val[1];

        if (__ref.key[0] == nullptr) {
            __ref.key[0] = next;
            __ref.val[0] = get_virtual_block <1> (prev);
            return __ref.val[0];
        }

        if (__ref.key[1] == nullptr) {
            __ref.key[1] = next;
            __ref.val[1] = get_virtual_block <2> (prev);
        }

        throw error("WTF ???");
    }


    /* Just create a temporary. */
    virtual_register *get_virtual() {
        return top_asm->create_virtual();
    }

    virtual_register *get_virtual(IR::non_literal *__temp) {
        auto &__ref = temp_map[__temp];
        if (!__ref) return __ref;
        else return get_virtual();
    }

    /* Load a constant to a random register. */
    virtual_register *get_virtual(IR::literal *__lit) {
        if (auto __ptr = dynamic_cast <IR::pointer_constant *> (__lit)) {
            auto *__ans = get_virtual();
            top_block->emplace_back(new load_high
                {__ans,const_cast <IR::global_variable *> (__ptr->var)});

            return __ans;
        } else {
            auto *__ans = get_virtual();
            top_block->emplace_back(new load_immediate 
                {__ans, __lit->get_constant_value()});
            return __ans;
        }
    }

    /**
     * @brief Return the virtual register of a definition.
     * @param __def Input definition.
     * @return The virtual register of the definition.
     * If __def = nullptr, return a new virtual register.
     * If __def = immediate, insert a load_immedate and return the virtual register.
     * If __def = non_literal, return the virtual register binded to it.
     * Else throw an exception.
     */
    virtual_register *get_virtual(IR::definition *__def) {
        if (dynamic_cast <IR::global_variable *> (__def)
        ||  dynamic_cast <IR::local_variable  *> (__def))
            runtime_assert("???");
        if (!__def) return get_virtual();
        else if (auto __tmp = dynamic_cast <IR::non_literal *> (__def))
            return get_virtual(__tmp);
        else
            return get_virtual(safe_cast <IR::literal *> (__def));
    }

    void create_entry(IR::function *);
};




}