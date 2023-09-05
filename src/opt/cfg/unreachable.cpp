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
    /* An unreachable function! */
    if(__func->is_unreachable()) return;

    /* Just for better performance. */
    block_set.reserve(__func->stmt.size());

    /* Add all unreachable nodes to worklist. */
    dfs(__entry,[](node *__node) { return __node->block->is_unreachable(); });

    /* Spread the unreachable blocks. */
    spread_unreachable();

    /* Remove all dead loop. */
    remove_dead_loop(__entry);

    /* Remove the unreachable blocks from function. */
    remove_unreachable(__func);

    /* Special case : unreachable! */
    if (__func->stmt.empty()) {
        __func->stmt = {__entry->block};
        __entry->block->stmt = {IR::create_unreachable()};
        __entry->next.clear();
        __entry->prev.clear();
        return;
    }

    dfs(__entry,[](node *__node) { return true; });
    /* Update the phi statement with source from unreachable source. */
    update_phi_branch_source();
}


void unreachable_remover::remove_unreachable(IR::function *__func) {
    std::vector <IR::block_stmt *> __blocks;
    for(auto __block : __func->stmt)
        if(block_set.erase(__block))
            __blocks.push_back(__block);
    __func->stmt = std::move(__blocks);
    runtime_assert("WTF in unreachable_remover",block_set.empty());
}


void unreachable_remover::spread_unreachable() {
    while(!work_list.empty()) {
        auto *__node = work_list.front(); work_list.pop_front();
        block_set.erase(__node->block);

        /* Spread undefined forward. */
        for(auto __next : __node->next) {
            remove_node_from(__next->prev,__node);
            if(__next->prev.empty()) work_list.push_back(__next);
        } __node->next.clear();

        /* Spread undefined backward. */
        for(auto __prev : __node->prev) {
            remove_node_from(__prev->next,__node);
            if(__prev->next.empty()) work_list.push_back(__prev);
        } __node->prev.clear();
    }
}


void unreachable_remover::update_phi_branch_source() {
    while(!work_list.empty()) {
        auto * __node = work_list.front(); work_list.pop_front();
        auto *&__last = __node->block->stmt.back();

        /* Change branches into jump if only one next. */
        if(auto *__br = dynamic_cast <IR::branch_stmt *> (__last)) {
            if(__node->next.size() == 1)
               __last = replace_branch(__br,__node->next[0]->block);
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


void unreachable_remover::remove_dead_loop(node *__entry) {
    std::unordered_set <node *> __visit;
    auto &&__dfs = [&](auto &&__self,node *__node) -> void {
        if (!__visit.insert(__node).second) return;
        /* Push these returning to the set. */
        if (dynamic_cast <IR::return_stmt *> (__node->block->stmt.back()))
            work_list.push_back(__node);

        for(auto __next : __node->next) __self(__self,__next);
    };

    /* Visit recursively. */
    __dfs(__dfs,__entry);

    /* Add all __node to set.  */
    __visit.clear();
    for(auto __node : work_list) __visit.insert(__node);
    while(!work_list.empty()) {
        auto __node = work_list.front(); work_list.pop_front();
        for(auto __prev : __node->prev)
            if(__visit.insert(__prev).second)
                work_list.push_back(__prev);
    }

    /* Remove those unreachable blocks. */
    for(auto __node : __visit) {
        if(block_set.count(__node->block)) {
            work_list.push_back(__node);
        } else for(auto __prev : __node->prev) {
            std::cerr << __node->block->data() << '\n';
            remove_node_from(__prev->next,__node);
        }
    }

    /* Copy the set back to the block_set. */
    if(block_set.size() != work_list.size()) {
        warning("Undefined behavior: Potential dead loop detected!");
        block_set.clear();
        for(auto __node : work_list) {
            std::cerr << __node->data() << '\n';
            block_set.insert(__node->block);
        }
    }

    work_list.clear();
}

}