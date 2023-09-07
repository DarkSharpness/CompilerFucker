#include "cfg.h"


namespace dark::OPT {


graph_simplifier::graph_simplifier(IR::function *__func,node *__entry) {
    /* This is the core function, which may eventually effect constant folding. */
    replace_const_branch(__func,__entry);

    if (__func->is_unreachable()) return;

    /* Collect all single phi usage before return. */
    collect_single_phi(__func,__entry);

    /* (This function is just a small opt!) Remove all single phi. */
    remove_single_phi(__func,__entry);

    /* (This function is just a small opt!) Finally compress jump. */
    compress_jump(__func,__entry);
}


void graph_simplifier::replace_const_branch(IR::function *__func,node *__entry) {
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
                __node->next.pop_back();
            }

        for(auto __next : __node->next) __self(__self,__next);
    };

    __dfs(__dfs,__entry);

    unreachable_remover {__func,__entry};
}


void graph_simplifier::compress_jump(IR::function *__func,node *__entry) {
    /* Add to the work list !!! */
    std::vector <node *> work_list;
    std::unordered_set <node*> visit;
    auto &&__dfs = [&](auto &&__self,node *__node) -> void {
        if (!visit.insert(__node).second) return;
        for(auto __next : __node->next) __self(__self,__next);
        work_list.push_back(__node);
    };
    __dfs(__dfs,__entry);

    /* Case that prev -> node is linked as a line. */
    auto &&__update_line = [&](node *__prev,node *__node) -> void {
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
    };

    /**
     * @brief Cases that a node has multiple incoming branches,
     * but only one outcoming edge (jump) as the node's only statement.
    */
    auto &&__update_branch = [](node *__node,node *__next) ->void {
        std::unordered_map <IR::block_stmt *,bool> conflict_map;
        /* Add all prev to the node. */
        for(auto __prev : __node->prev)
            conflict_map.emplace(__prev->block,false);
        for(auto __phi  : __next->block->get_phi_block()) {
            IR::definition *__new = nullptr;
            /* Move target to back of the vector. */
            for(auto &&__pair : __phi->cond) {
                if (__pair.label == __node->block) {
                    std::swap(__pair,__phi->cond.back());
                    __new = __pair.value; break;
                }
            } runtime_assert("nyan~~~?",__new != nullptr);

            /* Only available when value can be merged.  */
            for(auto [__old,__block] : __phi->cond)
                conflict_map[__block] |=
                    __old != merge_definition(__old,__new);
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
                __last = replace_branch(
                    safe_cast <IR::branch_stmt *> (__last),__next->block
                );
                runtime_assert("nyan~",remove_node_from(__prev->next,__node));
            } else {
                /* From unknown branch : update it now! */
                __next->prev.push_back(__prev);
                auto *__last = __prev->block->stmt.back();
                if (auto *__jump = dynamic_cast <IR::jump_stmt *> (__last)) {
                    __jump->dest    = __next->block;
                    __prev->next[0] = __next;
                } else { /* Branch replacement.  */
                    auto __br = safe_cast <IR::branch_stmt *> (__last);
                    __br->br[__br->br[0] != __node->block] = __next->block;
                    auto &__ref = __prev->next[0];
                    if (__ref == __node) __ref = __next;
                    else       __prev->next[1] = __next;
                }

                /* Insert new phi to one before back. */
                for(auto *__phi : __next->block->get_phi_block()) {
                    auto &__tmp = __phi->cond.back();
                    __phi->cond.push_back(__tmp);
                    __tmp.label = __prev->block;
                }
            }

            *__beg = *--__end;
        }

        /* All nodes have been removed...... */
        if (__bak == __beg) {
            /* Now this node is entirely out of CFG. */
            remove_node_from(__next->prev,__node);
            for(auto __phi : __next->block->get_phi_block())
                __phi->cond.pop_back();
            __node->prev.clear();
            __node->next.clear();
        } else __node->prev.resize(__beg - __bak);
    };


    while(!work_list.empty()) {
        auto *__node = work_list.back(); work_list.pop_back();
        if (__node->prev.size() == 0) continue; /* Entry node / empty case! */
        if (__node->prev.size() == 1 && __node->prev[0]->next.size() == 1) {
            auto *__prev = __node->prev[0];
            __update_line(__prev,__node);
        } else if (__node->next.size() == 1 && __node->block->stmt.size() == 1) {
            auto *__next = __node->next[0];
            if (__next->prev.size() == 1) __update_line(__node,__next);
            else /* May have multiple branching. */
                __update_branch(__node,__next);
        }
    }

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


void graph_simplifier::remove_single_phi(IR::function *__func,node *__entry) {
    /* Spread those single phi. */
    while(!work_list.empty()) {
        auto *__phi = work_list.front(); work_list.pop();
        auto __iter = use_map.find(__phi->dest);

        /* This node has been removed (unused now). */
        if(__iter == use_map.end()) continue;

        /* Complete the erasion. */
        auto __vec = std::move(__iter->second);
        use_map.erase(__iter);
        auto __ptr = get_phi_type(__phi);

        /* Update all uses. */
        for(auto __stmt : __vec) {
            __stmt->update(__phi->dest,__ptr);
            /* If single phi, just remove it! */
            if(auto __phi = dynamic_cast <IR::phi_stmt *> (__stmt);
                __phi && get_phi_type(__phi)) work_list.push(__phi);
        }

        /* Update use map. */
        if(auto __tmp = dynamic_cast <IR::temporary *> (__ptr)) {
            auto &__tar = use_map[__tmp];
            __tar.insert(__tar.end(),__vec.begin(),__vec.end());
        }
    }

    { /* Remove all useless phi. */
        std::vector <IR::node *> __vec;
        for(auto __block : __func->stmt) {
            size_t __cnt = 0; __vec.clear();
            for(auto __phi : __block->get_phi_block())
                if(use_map.count(__phi->dest))
                    __vec.push_back(__phi);
                else ++__cnt; /* Need to be removed. */
            if (!__cnt) continue;

            auto __beg = __block->stmt.cbegin();
            __block->stmt.erase(__beg,__beg + __cnt);

            /* Copy the useful data back. */
            memcpy(__block->stmt.data(),__vec.data(),__vec.size() * sizeof(IR::node **));

        }
    }
}


void graph_simplifier::collect_single_phi(IR::function *__func,node *__entry) {
    /* First, eliminate all single phi. */
    for(auto __block : __func->stmt) {
        for(auto __stmt : __block->stmt) {
            if(auto __phi = dynamic_cast <IR::phi_stmt *> (__stmt)) {
                if(get_phi_type(__phi)) work_list.push(__phi);
                auto __def = __phi->dest;
                for(auto __ptr : __phi->get_use()) {
                    auto __tmp = dynamic_cast <IR::temporary *> (__ptr);
                    /* Self reference is also forbidden! */
                    if (!__tmp || __tmp == __def) continue;
                    use_map[__tmp].push_back(__stmt);
                }
            } else {
                for(auto __use : __stmt->get_use()) {
                    auto __tmp = dynamic_cast <IR::temporary *> (__use);
                    if (!__tmp) continue;
                    use_map[__tmp].push_back(__stmt);
                }
            }
        }
    }
}

}