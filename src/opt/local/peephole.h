#pragma once

#include "optnode.h"
#include "calc.h"

#include <deque>
#include <unordered_set>
#include <unordered_map>

namespace dark::OPT {



/**
 * @brief A local optimizer that will optimize code
 * according to the local pattern.
 * 
*/
struct local_optimizer final : IR::IRvisitorbase {
    struct usage_info {
        IR::definition *new_def  = nullptr; /* New definition.      */
        IR::node       *def_node = nullptr; /* Definition node.     */
        /* If true, the def_node is a negative expression.          */
        /* Else , the def_node is the generator of new_def or null. */
        bool            neg_flag = false;
    };

    /**
     * Load  after load : Use previous load result.
     * Store after store: Remove previous store node.
     * Load  after store: Use previous store result.
     * Store after load : Nothing will happen.
     */
    struct memory_info {
        IR::memory_stmt *last = nullptr;   /* Last memory node.      */
        bool corrupted        = false;     /* Whether no sure store. */
    };

    struct custom_info {
        size_t op;
        IR::definition *lhs;
        IR::definition *rhs;
        custom_info(IR::binary_stmt *__bin) noexcept :
            op(__bin->op) , lhs(__bin->lvar) , rhs(__bin->rvar) {}

        custom_info(IR::get_stmt *__get) noexcept :
            op(1919810 + __get->mem) , lhs(__get->src), rhs(__get->idx) {}

        bool operator == (const custom_info &__info) const noexcept {
            return op == __info.op && lhs == __info.lhs && rhs == __info.rhs;
        }
    };

    struct custom_hash {
        size_t operator()(const custom_info &info) const noexcept {
            return info.op + (size_t)info.lhs + (size_t)info.rhs;
        }
    };

    struct usage_data : std::vector <IR::node *> {
        IR::node *def_node = nullptr;
    };

    enum class address_type : unsigned {
        GLOBAL  = 0, /* global variable */
        PHI     = 1, /* result from a phi stmt. */
        LOCAL   = 2, /* local variable. */
        LOAD    = 3, /* load from memory. */ 
        GET     = 4, /* get from another pointer. */    
        CALL    = 5, /* result from a function-call. */
        ARGV    = 6, /* function argument. */
        NEW     = 7, /* from malloc */
    };

    auto get_min_max(address_type __lhs,address_type __rhs) {
        struct my_pair {
            address_type min;
            address_type max;
        };
        if (static_cast <unsigned> (__lhs) < static_cast <unsigned> (__rhs))
            return my_pair {__lhs,__rhs};
        else
            return my_pair {__rhs,__lhs};
    }

    address_type get_address_info(IR::non_literal *__var);

    /* The def-use map of all temporaries. */
    std::unordered_map <IR::temporary *,usage_data> use_map;
    /* Use information. */
    std::unordered_map <IR::temporary *,usage_info> use_info;
    /* Memory information. */
    std::unordered_map <IR::non_literal *,memory_info> mem_info; 

    std::unordered_map <custom_info,IR::definition *,custom_hash>
        common_map; /* Map used for common sub-expression elimination. */

     /* Mapping of address information. */
    std::unordered_map <IR::non_literal *,address_type> addr_map; 

    std::vector <IR::definition *> __cache;
    constant_calculator calc;

    /* A set of elements to remove from. */
    std::unordered_set <IR::node *> remove_set;

    local_optimizer(IR::function *,node *);

    /**
     * @brief Replace common subexpression safely 
     * (literally and SFINAE-ly).
     * @param __stmt The target statment.
     * @return Whether the replacement succeeded.
     * If successful, then current expression will become
     * useless, and will be removed soon.
     */
    template <class T>
    auto replace_common(T *__stmt) -> std::enable_if_t
        <std::is_constructible_v <custom_info,T *>, bool> {
        /* Erase from the binary set if existed. */
        IR::temporary *__def = __stmt->get_def();
        if (auto [__iter,__flag] = common_map.try_emplace(__stmt,__def);
            __flag == false) {
            use_info[__def] = *get_use_info(__iter->second);
            return true;
        } return false;
    }

    /**
     * @brief Try to remove one temporary.
    */
    template <class T>
    auto try_remove(T *__stmt) -> std::enable_if_t
        <std::is_base_of_v <IR::node,T>, void> {
        auto *__def = __stmt->get_def();
        auto *__new = get_def_value(__def);
        if (__def == __new) return;
        for(auto __use : use_map[__def])
            __use->update(__def,__new);
        remove_set.insert(__stmt);
    }

    /* Just update a value. */
    inline IR::definition *get_def_value(IR::definition *__def) {
        auto __tmp = get_use_info(__def);
        if (!__tmp->neg_flag)
            return __tmp->new_def ? : __def;
        else
            return __tmp->def_node->get_def();
    }

    usage_info *get_use_info(IR::definition *);
    size_t may_share_address(IR::non_literal *,IR::non_literal *);
    bool update_bin(IR::binary_stmt *);

    void visitFunction(IR::function *) override {}
    void visitInit(IR::initialization *) override {}
    void visitBlock(IR::block_stmt *) override;

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

    ~local_optimizer() = default;
};


}

