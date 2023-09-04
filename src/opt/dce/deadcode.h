#pragma once

#include "optnode.h"
#include "mem2reg.h"
#include <stack>
#include <queue>
#include <unordered_set>
#include <unordered_map>

namespace dark::OPT {

inline bool is_removable_node(IR::node *__node) {
    if(dynamic_cast <IR::store_stmt *> (__node)) {
        return false;
    } else if(dynamic_cast <IR::call_stmt *> (__node)) {
        return false;
    } else if(dynamic_cast <IR::jump_stmt *> (__node)) {
        return false;
    } else if(dynamic_cast <IR::branch_stmt *> (__node)) {
        return false;
    } else if(dynamic_cast <IR::return_stmt *> (__node)) {
        return false;
    } else {
        return true;
    }
}


/**
 * @brief Eliminator for dead code in a function.
 * This is an unaggressive dead code eliminator.
 * 
 */
struct deadcode_eliminator {
    struct info_holder {
        /* The inner data. */
        IR::node *data = nullptr;
        /* Whether the node is removable. */
        bool removable = true;

        std::vector <IR::temporary *> uses;

        /* Init with a the definition node and all usage information. */
        void init(IR::node *__node) {
            /* Should only call once! */
            runtime_assert("Invalid SSA form!",data == nullptr);

            /* Init the data. */
            removable = is_removable_node(data = __node);
                
            auto __vec = __node->get_use();
            uses.reserve(__vec.size());
            for(auto __use : __vec)
                if(auto __tmp = dynamic_cast <IR::temporary *> (__use))
                    uses.push_back(__tmp);
        }
    };

    /* Mapping from defs to uses and real node. */
    std::unordered_map <IR::temporary *, info_holder *> info_map;
    /* This is the real data holder. */
    std::deque <info_holder>   info_list;
    /* This is the list of variables to remove. */
    std::queue <info_holder *> work_list;

    deadcode_eliminator(IR::function *,node *);

    /**
     * @brief Get the info tied to the temporary.
     * @param __temp The temporary (Non-nullptr)!
     * @return The info holder tied to the temporary.
     */
    info_holder *get_info(IR::temporary *__temp) {
        auto *&__ptr = info_map[__temp];
        if(!__ptr) __ptr = &info_list.emplace_back();
        return __ptr;
    }


    /**
     * @brief 
     * 
     * @param __temp The temporary defined by the node in SSA.
     * @return The info holder tied to the temporary.
     */
    info_holder *create_info(IR::node *__node) {
        auto *__def  = __node->get_def();
        return __def ? get_info(__def) : &info_list.emplace_back();
    }


};


}