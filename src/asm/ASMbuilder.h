#pragma once

#include "IRbase.h"
#include "ASMnode.h"

#include <unordered_map>

namespace dark::ASM {


struct ASMbuilder final : IR::IRvisitorbase {
    struct getelement_info {
        IR::non_literal *base; /* Base definition. */
        size_t         offset; /* Relative offset from base. */
    };
    struct branch_node {
        IR::block_stmt *key[2] = {};
        block          *val[2] = {};
    };
    struct phi_info {
        std::vector <virtual_register *> def;
        std::vector <value_type>         use;
    };
    struct usage_info {
        IR::node *def = nullptr;
        ssize_t count = 0;
    };

    ASMbuilder(std::vector <IR::initialization> &global_variables,
               std::vector <IR::function>       &global_functions) {
        for(auto &&__var : global_variables) visitInit(&__var);

        for(auto &&__func : global_functions) visitFunction(&__func);

        for(auto &&__func : global_functions)
            global_info.function_list.push_back(&func_map.at(&__func));

        build_rodata();
    }


    IR::function   *top_func;  /* Top IR  function. */
    function       *top_asm;   /* Top ASM function. */
    IR::block_stmt *top_stmt;  /* Top IR  block statement. */
    block          *top_block; /* Top ASM block statement. */

    std::unordered_map <IR::definition *, size_t> branch_count;
    std::unordered_map <IR::definition *, usage_info> use_map;
    std::unordered_map <IR::function *,function>     func_map;
    std::unordered_map <IR::block_stmt *,block >    block_map;
    std::unordered_map <IR::non_literal *, virtual_register *>  temp_map;
    std::unordered_map <IR::temporary *, getelement_info> getelement_map;
    std::unordered_map <IR::local_variable *, ssize_t>        offset_map;
    std::unordered_map <IR::block_stmt *,branch_node>        branch_map;
    std::unordered_map <block *,phi_info> phi_map;
    std::unordered_set <IR::node *> tail_call_set;

    global_information global_info;

    template <size_t __off>
    block *get_virtual_block(IR::block_stmt *__old) {
        static_assert(__off < 10 && __off > 0);
        size_t __hash = std::hash <IR::block_stmt *> () (__old) + __off;
        auto *__block = reinterpret_cast <IR::block_stmt *> (__hash);
        auto  __name  = string_join(__old->label,"_v_",char(__off + '0'));
        return &block_map.try_emplace(__block,__name,__old).first->second;
    }

    block *get_block(IR::block_stmt *__block) {
        return &block_map.try_emplace(__block,__block->label,__block).first->second;
    }

    /* No renaming is done. */
    function *get_function(IR::function *__func) {
        auto *__ans = &func_map[__func];
        if (__ans->name.empty()) {
            __ans->name = __func->name;
            __ans->func_ptr = __func;
        } return __ans;
    }

    static size_t get_builtin_index(IR::function *) noexcept;

    /* Return the address possibly from getelementptr. */
    pointer_address get_pointer_address(IR::non_literal *);

    /* Return the inner address(value). */
    value_type get_value(IR::non_literal *);

    /* Return the inner address(value.) */
    value_type get_value(IR::definition *);

    /* Just create a virtual register. */
    virtual_register *create_virtual();

    /* Return the binded virtual register for temporary/arguments. */
    virtual_register *get_virtual(IR::non_literal *);

    /* Force to load value to a register. */
    Register *force_register(IR::definition *);

    /* Get the edge block between 2 block. */
    block *get_edge(IR::block_stmt *,IR::block_stmt *);

    void build_rodata();

    function_node *builtin_inline(IR::call_stmt *,Register *);

    void create_entry(IR::function *);
    void pre_scanning(IR::function *);

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

    block *link(block *__prev,block *__next) {
        __prev->next.push_back(__next);
        __next->prev.push_back(__prev);
        return __next;
    }

    node *try_assign(Register *,value_type);
    void resolve_phi(phi_info &);
};




}