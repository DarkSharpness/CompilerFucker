#include "cfg.h"
#include <algorithm>

namespace dark::OPT {


/**
 * @brief Just remove all constant branches.
 * The CFG graph will be changed, but still be valid.
 * This pass depends on unreachable_remover to be valid.
 * So no need to call unreachable_remover after this pass.
 * 
 */
branch_cutter::branch_cutter(IR::function *__func,node *__entry) {
    std::unordered_set <node *> __set;
    auto &&__dfs = [&](auto &&__self,node *__node) -> void {
        if(!__set.insert(__node).second) return;
        auto *__block = __node->block;
        auto *&__last = __block->stmt.back();

        /* Unreachable settings~ */
        if(auto *__br = dynamic_cast <IR::branch_stmt *> (__last))
            if (auto __cond = dynamic_cast <IR::boolean_constant *> (__br->cond)) {
                runtime_assert("~~~",__node->next.size() == 2);
                auto &__next = __node->next[1];
                if (__next->block != __br->br[__cond->value])
                    std::swap(__next,__node->next[0]);
                runtime_assert("~~~",remove_node_from(__next->prev,__node));
                for(auto __phi : __next->block->get_phi_block())
                    __phi->update(__block,__next->block);
                __node->next.pop_back();
            }

        for(auto __next : __node->next) __self(__self,__next);
    };

    __dfs(__dfs,__entry);
    unreachable_remover {__func,__entry};
}


/**
 * @brief Update the branch prev -> node to prev->next.
 * It will modify informatio with prev only.
 */
void branch_compressor::update_branch
    (IR::branch_stmt *__br,node *__prev,node *__node,node *__next) {
    __br->br[__br->br[0] != __node->block] = __next->block;
    auto &__ref = __prev->next[0];
    if (__ref == __node) __ref = __next;
    else       __prev->next[1] = __next;
}


/**
 * @brief A mild to radical branch compressor.
 * It will compress all jumps to jump , some branches to jump if possible.
 * Note that after this pass , there may be some unreachable node in the
 * CFG graph, so you may call unreachable_remover to remove them.
 * 
 */
branch_compressor::branch_compressor(IR::function *__func,node *__entry) {
    /* Dfs to build up the work-list and visit. */
    auto &&__dfs = [&](auto &&__self,node *__node) -> void {
        if (!visit.insert(__node).second) return;
        for(auto __next : __node->next) __self(__self,__next);
        work_list.push_back(__node);
    };
    __dfs(__dfs,__entry);

    /* Merge those identical exit-only blocks. */
    [&]() -> void {
        std::unordered_map <IR::definition *,node *> __map;
        for(auto __node : visit) 
            if(auto __ret = dynamic_cast <IR::return_stmt *>
                (__node->block->stmt.front())) {
                auto [__iter,__bool] = __map.try_emplace(__ret->rval,__node);
                if (__bool) continue; /* First such exit. */

                /* Relinking! */
                auto __next = __iter->second;
                for(auto __prev : __node->prev) {
                    __next->prev.push_back(__prev);
                    auto *&__last = __prev->block->stmt.back();
                    if (auto *__jump = dynamic_cast <IR::jump_stmt *> (__last)) {
                        __jump->dest = __next->block;
                        __prev->next[0] = __next;
                        continue;
                    }

                    /* Branch case now. */
                    auto *__br = safe_cast <IR::branch_stmt *> (__last);
                    if (__br->br[0] == __next->block || __br->br[1] == __next->block) {
                        __prev->next = {__next};
                        __last = replace_branch(__br,__next->block);
                    } else {  /* Branch replacement.  */
                        update_branch(__br,__prev,__node,__next);
                    }
                }
                /* Clear all prev data. */
                __node->prev.clear();
            }
    }();

    /* Collect all def use data. */
    std::vector <block_info> count_pool(__func->stmt.size());
    [&]() -> void {
        auto *__ptr = count_pool.data();
        for(auto __block : __func->stmt) {
            for(auto __stmt : __block->stmt) {
                auto *__def = __stmt->get_def();
                if (__def) {
                    auto &__ref = use_map[__def];
                    __ref.def_node = __stmt;
                    __ref.block    = __ptr;
                }
                for(auto __use : __stmt->get_use())
                    if (auto __tmp = dynamic_cast <IR::temporary *> (__use))
                        ++use_map[__tmp].ref_count;
            } (__ptr++)->init(__block);
        }
    }();

    /* Remove useless temporary for branches. */
    auto &&__remove = [&](IR::node *__stmt) -> void {
        if (auto __call = dynamic_cast <IR::call_stmt *> (__stmt)) {
            if (!__call->func->is_builtin || !__call->func->inout_state)
                return; /* Side effective call stmt can't be removed. */
        } if (!__stmt) return; /* Empty case. */

        /* Otherwise, for an unused node, it should be recycled! */
        for(auto __use : __stmt->get_use()) try_remove_useless(__use);

        auto &__ref = use_map[__stmt->get_def()];
        auto &__vec = __ref.block->block->stmt;

        /* Erase from last can be faster. */
        __vec.erase(std::find(__vec.rbegin(),__vec.rend(),__stmt).base() - 1);
        if (__vec.size() == 1) {
            __vec = {__vec.back()}; /* Only leave the control flow. */
            work_list.push_back(__ref.block->data);
        }
    };

    /* Compress useless branches. */
    auto &&__compress = [&](node *__node) -> void {
        if (__node->prev.size() == 0) return; /* Entry node / empty case! */
        if (__node->prev.size() == 1 && __node->prev[0]->next.size() == 1) {
            /* If single phi, we can't compress the node! */
            compress_line(__node->prev[0],__node);
        } else if (__node->next.size() == 1 && __node->block->stmt.size() == 1) {
            auto *__next = __node->next[0];
            if (__next->prev.size() == 1 && compress_line(__node,__next)) return;
            compress_branch(__node,__next);
        }
    };

    while(!work_list.empty()) {
        do {
            auto *__node = work_list.back(); work_list.pop_back();
            __compress(__node);
        } while(!work_list.empty());
        while(!remove_list.empty()) {
            auto *__node = remove_list.back(); remove_list.pop_back();
            __remove(__node);
        }
    }

    remove_block(__func,__entry);
}


/**
 * @brief Compress 2 nodes that are in a line.
 */
bool branch_compressor::compress_line(node *__prev,node *__node) {
    if(dynamic_cast <IR::phi_stmt *> (__node->block->stmt.front()))
        return false; /* Unable to compress a single-phi block. */

    /* Update the CFG graph and related phi. */
    __node->prev.clear();
    __prev->next = std::move(__node->next);

    for(auto __next : __prev->next) {
        for(auto &__temp : __next->prev)
            if(__temp == __node) __temp = __prev;
        for(auto __phi : __next->block->get_phi_block())
            __phi->update(__node->block,__prev->block);
    }

    /* Copy the data. */
    auto &__stmt = __prev->block->stmt;
    __stmt.pop_back(); /* Pop the useless flow now. */
    auto &__data = __node->block->stmt;
    __stmt.insert(__stmt.end(),__data.begin(),__data.end());
    for(auto __stmt : __data) {
        auto __tmp  = __stmt->get_def();
        if (!__tmp) continue;
        auto &__ref = use_map[__tmp];
        __ref.block->init(__prev->block);
    }

    work_list.push_back(__prev);
    return true;
}

/**
 * @brief Compress 2 node that are in a branch
 * @param __node The node to with only one exit.
 * @param __next The node to with multiple entry.
 */
void branch_compressor::compress_branch(node *__node,node *__next) {
    std::unordered_map <IR::block_stmt *,bool> conflict_map;
    /* Add all prev to the conflict_map. */
    for(auto __prev : __next->prev)
        conflict_map.emplace(__prev->block,false);

    auto __phi_vec = __next->block->get_phi_block();
    for(auto __phi  : __phi_vec) {
        IR::definition *__new = nullptr;
        /* Move target to back of the vector. */
        for(auto &&__pair : __phi->cond) {
            if (__pair.label == __node->block) {
                __new = __pair.value;
                std::swap(__pair,__phi->cond.back());
                break;
            }
        } runtime_assert("nyan~~~?",__new != nullptr);

        /* Only available when value can be merged.  */
        for(auto [__old,__block] : __phi->cond)
            conflict_map[__block] |=
                (__old != merge_definition(__old,__new));
    }

    auto __beg = __node->prev.begin();
    auto __bak = __beg;
    auto __end = __node->prev.end();

    while(__beg != __end) {
        auto __prev = *__beg;
        auto __iter = conflict_map.find(__prev->block);
        if (__iter != conflict_map.end()) {
            if (__iter->second) { ++__beg; continue; }

            /* From existed branch : branch to jump.  */
            auto *&__last = __prev->block->stmt.back();
            auto  *__br   = safe_cast <IR::branch_stmt *> (__last);
            try_remove_useless(__br->cond);

            __last = replace_branch(__br,__next->block);

            /* Maybe prev node can be updated. */
            work_list.push_back(__prev);
            runtime_assert("nyan~",remove_node_from(__prev->next,__node));
        } else {
            /* From unknown branch : update it now! */
            __next->prev.push_back(__prev);
            auto *__last = __prev->block->stmt.back();
            if (auto *__jump = dynamic_cast <IR::jump_stmt *> (__last)) {
                __jump->dest    = __next->block;
                __prev->next[0] = __next;
            } else { /* Branch replacement.  */
                auto *__br = safe_cast <IR::branch_stmt *> (__last);
                update_branch(__br,__prev,__node,__next);
            }

            /* Insert new phi to one before back. */
            for(auto *__phi : __phi_vec) {
                __phi->cond.push_back(__phi->cond.back());
                (__phi->cond.end() - 2)->label = __prev->block;
            }
        }

        *__beg = *--__end;
    }

    /* All nodes have been removed...... */
    if (__bak == __beg) {
        /* Now this node is entirely out of CFG. */
        remove_node_from(__next->prev,__node);
        for(auto __phi : __phi_vec)
            __phi->cond.pop_back();
        __node->prev.clear();
        __node->next.clear();
    } else __node->prev.resize(__beg - __bak);
}


/**
 * @brief Remove all unreachable node in the last step.
 */
void branch_compressor::remove_block(IR::function *__func,node *__entry) {
     /* Set of all unreachable node. */
    std::unordered_set <IR::block_stmt *> __set;
    for(auto __node : visit)
        if (__node->prev.empty() && __node != __entry)
            __set.insert(__node->block);

    /* Remove all unreachable node. */
    std::vector <IR::block_stmt *> __final;
    for(auto __block : __func->stmt)
        if (!__set.count(__block)) __final.push_back(__block);

    __func->stmt = std::move(__final);
}

/* Try to find dead code and remove it. */
void branch_compressor::try_remove_useless(IR::definition *__def) {
    if (auto *__tmp = dynamic_cast <IR::temporary *> (__def)) {
        auto &__ref = use_map[__tmp];
        if (--__ref.ref_count == 0)
            remove_list.push_back(__ref.def_node);
    }
}


}