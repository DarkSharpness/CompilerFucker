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

    /* Visit the node from entry. */
    update_dfs_front(__entry);

    /* Visit the node from exits. */
    update_dfs_reverse();

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

    /* Update the phi statement with source from unreachable source. */
    update_phi_branch_source();
}


void unreachable_remover::remove_unreachable(IR::function *__func) {
    std::vector <IR::block_stmt *> __blocks;
    for(auto __block : __func->stmt)
        if(block_set.count(__block))
            __blocks.push_back(__block);
    __func->stmt = std::move(__blocks);
}


void unreachable_remover::update_dfs_front(node *__entry) {
    auto &&__dfs = [&](auto &&__self,node *__node) -> void {
        /* It will not go into the set. */
        if (__node->block->is_unreachable()) return;
        if (!block_set.insert(__node->block).second) return;
        if (dynamic_cast <IR::return_stmt *> (__node->block->stmt.back()))
            work_list.push_back(__node);
        for(auto __next : __node->next) __self(__self,__next);
    };

    /* Search from entrance. */
    __dfs(__dfs,__entry);
}


void unreachable_remover::update_dfs_reverse() {
    decltype(block_set) __old_set;
    __old_set.swap(block_set);
    decltype(work_list) __old_list;
    __old_list.swap(work_list);

    auto &&__dfs = [&](auto &&__self,node *__node) -> void {
        if (!__old_set.count(__node->block)) return;
        if (!block_set.insert(__node->block).second) return;
        work_list.push_back(__node);
        for(auto __prev : __node->prev) __self(__self,__prev);
    };

    /* Search from all exits. */
    for(auto __node : __old_list) __dfs(__dfs,__node);
}


void unreachable_remover::update_phi_branch_source() {
    /* Core update algorithm (SFINAE safe~) */
    auto &&__update = [&](auto &__vec,auto &&__Func) -> std::enable_if_t
        <std::is_convertible_v <decltype(__Func(*__vec.end())),bool>> {
        auto __beg = __vec.begin();
        auto __end = __vec.end();
        auto __bak = __beg;
        while(__beg != __end) {
            if (__Func(*__beg))
                *__beg = *--__end;
            else ++__beg;
        } __vec.resize(__beg - __bak);
    };

    for(auto *__node : work_list) {
        auto &&__CFG = [&](node *__node) -> bool {
            return !block_set.count(__node->block);
        };
        auto &&__PHI = [=](auto &__pair) -> bool {
            for(auto __prev : __node->prev)
                if(__prev->block == __pair.label)
                    return false;
            return true;
        };

        /* First,update CFG info. */
        __update(__node->prev,__CFG);
        __update(__node->next,__CFG);
        auto &__last = __node->block->stmt.back();

        /* Change branches into jump if only one next. */
        if(auto *__br = dynamic_cast <IR::branch_stmt *> (__last))
            if(__node->next.size() == 1)
               __last = replace_branch(__br,__node->next[0]->block);

        /* Remove unsourced phi node. */
        for(auto *__phi : __node->block->get_phi_block())
            __update(__phi->cond,__PHI);

    }
}


}