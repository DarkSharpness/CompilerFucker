#include "deadcode.h"


namespace dark::OPT {


/**
 * @brief Eliminator for aggressive dead code in a function.
 * It will analyze the potential effect in the reverse graph.
 * If a block's definition is not linked to any useful block,
 * it will be removed.
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

    for(auto __block : __func->stmt) {
        if (!live_set.count(__block->get_impl_ptr <node> ()))
            std::cerr << "Dead : " << __block->label << '\n';
    }
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


}