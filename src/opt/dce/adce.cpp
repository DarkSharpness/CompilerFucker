#include "deadcode.h"


namespace dark::OPT {


/* Build up the dom tree. */
void build_dom_tree(IR::function *__func,
    const std::unordered_set <node *> &live_set) {
    struct dom_info {
        node   *idom = nullptr;
        size_t depth = 0;
    };
    std::unordered_map <node *,dom_info> dom_map;

    auto &&__dfs = [&](auto &&__self,node *__node) -> size_t {
        auto &__info = dom_map[__node];
        if (__info.depth) return __info.depth;
        /* Have been visited. */
        for(auto __prev : __node->dom) {
            if (__prev == __node) continue;
            size_t __dep = __self(__self,__prev) + 1;
            if (__dep > __info.depth) {
                __info.depth = __dep;
                __info.idom  = __prev;
            }
        }

        auto __idom = __info.idom;
        if (!__idom) __info.depth = 1, runtime_assert("??",live_set.count(__node) > 0);

        __node->fro = { live_set.count(__node) ? __node : __idom->fro[0]};
        return __info.depth;
    };

    for(auto __block : __func->stmt)
        __dfs(__dfs,__block->get_impl_ptr <node> ());
}

/**
 * @brief Eliminator for aggressive dead code in a function.
 * It will analyze the potential effect in the reverse graph.
 * If a block's definition is not linked to any useful block,
 * it will be removed.
 * 
 * Note that the CFG node graph will be disabled after this pass.
 * So always rebuild CFG after this pass.
 * 
*/
aggressive_eliminator::aggressive_eliminator
    (IR::function *__func,node *__entry) {
    if(__func->is_unreachable()) return;

    auto &&__rule = [](IR::node *__node) -> bool {
        if(dynamic_cast <IR::store_stmt *> (__node)) {
            return false;
        } else if(auto __call = dynamic_cast <IR::call_stmt *> (__node)) {
            return !__call->func->is_side_effective();
        } else if(dynamic_cast <IR::return_stmt *> (__node)) {
            return false;
        } else { /* Of course safe! */
            return true;
        }
    };

    collect_useful(__func,__rule);
    replace_undefined();
    spread_live();

    for(auto __block : __func->stmt)
        __block->get_impl_ptr <node> ()->fro.clear();

    build_dom_tree(__func,live_set);
    update_dead(__func);
    remove_useless(__func);
}


void aggressive_eliminator::spread_live() {
    auto &&__spread_useful = [this](info_holder *__info) -> void {
        if (__info->removable) return;
        if (live_set.insert(__info->node).second)
            node_list.push(__info->node);    
        for(auto *__use : __info->uses) {
            auto __prev = info_map[__use];
            if (__prev->removable) {
                __prev->removable = false;
                work_list.push(__prev);
            }
        }
    };

    auto &&__spread_lively = [this](node *__node) -> void {
        for(auto __prev : __node->prev) {
            if (__prev->next.size() == 1) {
                if (live_set.insert(__prev).second)
                    node_list.push(__prev);
            }
        }
        for(auto __prev : __node->fro) {
            auto * __last = __prev->block->stmt.back();
            if (auto __br = dynamic_cast <IR::branch_stmt *> (__last)) {
                auto __temp = dynamic_cast <IR::temporary *> (__br->cond);
                if (!__temp) continue;
                auto __info = info_map[__temp];
                if (__info->removable) {
                    __info->removable = false;
                    work_list.push(__info);
                }
            }
        }
    };

    while(!work_list.empty()) {
        do {
            auto __info = work_list.front(); work_list.pop();
            __spread_useful(__info);
        } while (!work_list.empty());
        while(!node_list.empty()) {
            auto __node = node_list.front(); node_list.pop();
            __spread_lively(__node);
        }
    }

}


/* Update all dead blocks's branch condition. */
void aggressive_eliminator::update_dead(IR::function *__func) {
    /* Do not modify the CFG! */
    for(auto &__info : info_list)
        if (dynamic_cast <IR::jump_stmt *>   (__info.data)
        ||  dynamic_cast <IR::branch_stmt *> (__info.data))
            __info.removable = false;

    for(auto __block : __func->stmt) {
        auto *&__last = __block->stmt.back();
        if (auto *__jump = dynamic_cast <IR::jump_stmt *> (__last)) {
            auto *__next = __jump->dest->get_impl_ptr <node> ();
            __jump->dest = {__next->fro[0]->block};
        } else if (auto *__br = dynamic_cast <IR::branch_stmt *> (__last)) {
            auto __br_0 = __br->br[0]->get_impl_ptr <node> ();
            auto __br_1 = __br->br[1]->get_impl_ptr <node> ();
            __br->br[0] = {__br_0->fro[0]->block};
            __br->br[1] = {__br_1->fro[0]->block};
            if (__br->br[0] == __br->br[1])
                __last  = replace_branch(__br,__br->br[0]);
        }
    }
}

}