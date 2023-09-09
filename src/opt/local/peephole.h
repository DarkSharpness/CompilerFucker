#pragma once

#include "optnode.h"

#include <deque>
#include <unordered_map>

namespace dark::OPT {


/* That's an interesting implement. */
struct polynomial : IR::definition {
    /* Specially, if op == 0xff, then the polynomial is removed. */
    decltype(IR::binary_stmt::op) op;
    IR::definition *lval;
    IR::definition *rval;

    IR::wrapper get_value_type() const override
    { runtime_assert(""); return {}; }
    std::string data() const override
    { runtime_assert(""); return {}; }
    ~polynomial() override = default;
};


/**
 * @brief A local optimizer that will optimize code
 * according to the local pattern.
 * 
*/
struct local_optimizer {
    struct usage_info {
        std::vector <IR::node *>  use_list;  /* Use list. */
        IR::definition *new_def  = nullptr;  /* Potential new definition. */
        IR::node       *def_node = nullptr;  /* Definition node. */
    };

    std::unordered_map <IR::definition *,usage_info>  use_info; /* Use information. */

    std::deque <IR::node *>  work_list;  /* Worklist for peephole~ */

    local_optimizer(IR::function *,node *);

};

}

