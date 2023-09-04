#include "cfg.h"
#include <algorithm>

namespace dark::OPT {


/* Erase the node from the vector and return whether success. */
bool remove_node_from(std::vector <node *> &__vec,node *__node) {
    auto __beg = __vec.begin();
    auto __end = __vec.end();
    while(__beg != __end) {
        if(*__beg == __node) {
            *__beg = __end[-1];
            __vec.pop_back();
            return true;
        } else ++__beg;
    } return false;
}


unreachable_remover::unreachable_remover
    (IR::function *__func,node *__entry) {

    /* Add all unreachable nodes to worklist. */
    block_set.reserve(__func->stmt.size());
    dfs(__entry,[](node *__node) { return __node->block->is_unreachable(); });

    /* Spread the unreachable blocks. */
    while(!work_list.empty()) {
        auto *__node = work_list.front(); work_list.pop();
        block_set.erase(__node->block);

        /* Spread undefined forward. */
        for(auto __next : __node->next) {
            remove_node_from(__next->prev,__node);
            if(__next->prev.empty()) work_list.push(__next);
        } __node->next.clear();

        /* Spread undefined backward. */
        for(auto __prev : __node->prev) {
            remove_node_from(__prev->next,__node);
            if(__prev->next.empty()) work_list.push(__prev);
        } __node->prev.clear();
    }

    { /* Remove the unreachable blocks from function. */
        std::vector <IR::block_stmt *> __blocks;
        for(auto __block : __func->stmt)
            if(block_set.count(__block))
                __blocks.push_back(__block);
        __func->stmt = std::move(__blocks);
        block_set.clear();
    }

    /* Add all (of course reachable) nodes to worklist. */
    dfs(__entry,[](void *) { return true; });

    /* Update the phi statement */
    while(!work_list.empty()) {
        auto * __node = work_list.front(); work_list.pop();
        auto *&__stmt = __node->block->stmt.back();
        if(auto *__br = dynamic_cast <IR::branch_stmt *> (__stmt)) {
            /* Change branches into jump! */
            if(__node->next.size() == 1)
               __stmt = replace_branch(__br,__node->next[0]->block);
        }
        for(auto *__phi : __node->block->get_phi_block()) {
            auto  __beg = __phi->cond.begin();
            auto  __bak = __beg;
            auto  __end = __phi->cond.end();
            while(__beg != __end) {
                if(!block_set.count(__beg->label)) {
                    /* phi from this branch will vanish forever. */
                    *__beg = *--__end;                
                } else ++__beg;
            } __phi->cond.resize(__end - __bak);
        }
    }
}


}