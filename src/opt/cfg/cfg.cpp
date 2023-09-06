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
    // compress_jump(__func,__entry);
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
    /// TODO: Compress all those jumps:
    /// Jumps to same exit, jumps to jumps. Jump to non-conflict phis.
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