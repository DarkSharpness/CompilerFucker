#include "sccp.h"



namespace dark::OPT {


/**
 * @brief This is helper class that will remove all single phi.
 * It will replace single phi from its source.
 * It will not change CFG graph, but just remove all single phi.
 * 
*/
single_killer::single_killer(IR::function *__func,node *) {
    /* Collect all single phi usage before return. */
    collect_single_phi(__func,nullptr);
    /* (This function is just a small opt!) Remove all single phi. */
    remove_single_phi(__func,nullptr);
}


/**
 * @brief Merge all phi value into one.
 * @param __phi Phi statement input.
 * @return The common value of all phi.
 * If non-null, this is a single phi.
 */
IR::definition *single_killer::merge_phi_value(IR::phi_stmt *__phi) {
    IR::definition *__tmp = nullptr;

    for(auto [__use,__] : __phi->cond) {
        /**
         * @brief Simple rule as below:
         * Self = undef.
         * undef + undef = undef.
         * undef + def   = single phi def.
         * def   + def   = non-single phi.
        */
        if(dynamic_cast <IR::undefined *> (__use)) continue;
        else if(__tmp == nullptr) __tmp = __use;
        /* Multiple definition: not a zero/ single phi! */
        else if(__tmp != __use) return nullptr;
    }

    return __tmp ? __tmp : IR::create_undefined(__phi->dest->type);
}


void single_killer::collect_single_phi(IR::function *__func,node *) {
    /* First, eliminate all single phi. */
    for(auto __block : __func->stmt) {
        for(auto __stmt : __block->stmt) {
            if(auto __phi = dynamic_cast <IR::phi_stmt *> (__stmt)) {
                if(merge_phi_value(__phi)) work_list.push(__phi);
                auto __def = __phi->dest;
                for(auto [__val,__] : __phi->cond) {
                    auto __tmp = dynamic_cast <IR::temporary *> (__val);
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


void single_killer::remove_single_phi(IR::function *__func,node *) {
    /* Spread those single phi. */
    while(!work_list.empty()) {
        auto *__phi = work_list.front(); work_list.pop();
        auto __iter = use_map.find(__phi->dest);

        /* This node has been removed (unused now). */
        if(__iter == use_map.end()) continue;

        /* Complete the erasion. */
        auto __vec = std::move(__iter->second);
        use_map.erase(__iter);
        auto __ptr = merge_phi_value(__phi);

        /* Update all uses. */
        for(auto __stmt : __vec) {
            __stmt->update(__phi->dest,__ptr);
            /* If single phi, just remove it! */
            if(auto __phi = dynamic_cast <IR::phi_stmt *> (__stmt);
                __phi && merge_phi_value(__phi)) work_list.push(__phi);
        }

        /* Update use map. */
        if(auto __tmp = dynamic_cast <IR::temporary *> (__ptr)) {
            auto &__tar = use_map[__tmp];
            __tar.insert(__tar.end(),__vec.begin(),__vec.end());
        }
    }

    [&,__func]() -> void { /* Remove all useless phi. */
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
    }();
}


}