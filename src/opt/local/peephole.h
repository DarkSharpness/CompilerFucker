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
struct local_optimizer {
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
        IR::node *last = nullptr;   /* Last memory node. */
    };

    /* Use information. */
    std::unordered_map <IR::temporary * , usage_info>  use_info;
    /* Memory information. */
    std::unordered_map <IR::non_literal *,memory_info> mem_info; 
    /* A set of elements to remove from. */
    std::unordered_set <IR::node *> remove_set;

    struct binary_info {
        size_t op;
        IR::definition *lhs;
        IR::definition *rhs;
        binary_info(IR::binary_stmt *__bin) noexcept :
            op(__bin->op) , lhs(__bin->lvar),rhs(__bin->rvar) {}
        bool operator == (const binary_info &__info) const noexcept {
            return op == __info.op && lhs == __info.lhs && rhs == __info.rhs;
        }
    };
    struct binary_hash {
        size_t operator()(const binary_info &info) const noexcept {
            return info.op + (size_t)info.lhs + (size_t)info.rhs;
        }
    };

    std::unordered_map <binary_info,IR::definition *,binary_hash> binary_map;

    std::vector <IR::definition *> __cache;
    constant_calculator calc;

    local_optimizer(IR::function *,node *);
    void optimize(IR::block_stmt *);

    /* Try to eliminate common expression. */
    bool replace_binary(IR::binary_stmt *__stmt) {
        /* Erase from the binary set if existed. */
        if (auto [__iter,__flag] = binary_map.try_emplace(__stmt,__stmt->dest);
            __flag == false) {
            use_info[__stmt->dest] = *get_use_info(__iter->second);
            return true;
        } return false;
    }

    usage_info *get_use_info(IR::definition *);

    bool update_bin(IR::binary_stmt *);
    void update_cmp(IR::compare_stmt *);
    void update_load(IR::load_stmt *);
    void update_store(IR::store_stmt *);
    void update_get(IR::get_stmt *);
    void update_br(IR::branch_stmt *);
};


}

