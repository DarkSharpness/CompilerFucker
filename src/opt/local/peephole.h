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
        IR::definition *new_def  = nullptr; /* New definition.    */
        IR::node       *def_node = nullptr; /* Definition node.   */
        union {
            /* If true, the def node is a just negative expression. */
            bool        neg_flag;
            /* If true, the def node is a just not (== false) expression. */
            bool        not_flag;
            bool        __init__ = false;
        };
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
    std::unordered_map <IR::temporary *,usage_info>  use_info;
    /* Memory information. */
    std::unordered_map <IR::non_literal *,memory_info> mem_info; 
    /* A set of elements to remove from. */
    std::unordered_set <IR::node *> remove_set;

    std::vector <IR::definition *> __cache;
    constant_calculator calc;


    local_optimizer(IR::function *,node *);
    void optimize(IR::block_stmt *);

    usage_info *get_use_info(IR::definition *);

    void update_bin(IR::binary_stmt *);
    void update_cmp(IR::compare_stmt *);
    void update_load(IR::load_stmt *);
    void update_store(IR::store_stmt *);
    void update_get(IR::get_stmt *);
    void update_br(IR::branch_stmt *);
};


}

