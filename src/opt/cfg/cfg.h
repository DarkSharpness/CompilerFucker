#pragma once

#include "optnode.h"
#include <queue>
#include <unordered_set>

namespace dark::OPT {

/* This is helper class that removes all useless nodes. */
struct unreachable_remover {
    std::unordered_set <IR::block_stmt *> block_set;
    std::queue <node *> work_list;
    unreachable_remover(IR::function *,node *);

    /**
     * @brief Add all nodes that satisfies the condition to work_list.
     * @tparam _Func Condition function type.
     * @param __node The node that is being visited.
     * @param __func Condition function.
     */
    template <class _Func>
    void dfs(node *__node,_Func &&__func) {
        /* All ready visited. */
        if(block_set.insert(__node->block).second == false) return;
        if(__func(__node)) work_list.push(__node);
        for(auto __next : __node->next) dfs(__next,std::forward <_Func> (__func));
    }

};


}