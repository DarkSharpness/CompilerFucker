#include "cfg.h"


namespace dark::OPT {

CFGsimplifier::CFGsimplifier(IR::function *__func,node *__entry) {
    /* This is the core function, which may eventually effect constant folding. */
    replace_const_branch(__func,__entry);

    /* (This function is just a small opt!) Remove all single phi. */
    remove_single_phi(__func,__entry);

    /* (This function is just a small opt!) Finally compress jump. */
    compress_jump(__func,__entry);
}



void CFGsimplifier::replace_const_branch(IR::function *__func,node *__entry) {
    std::unordered_set <node *> __set;
    auto &&__dfs = [&](auto &&__self,node *__node) -> void {
        if(!__set.insert(__node).second) return;
        auto *__block = __node->block;
        auto *&__last = __block->stmt.back();
        /* Unreachable settings~ */
        if(auto *__br = dynamic_cast <IR::branch_stmt *> (__last))
            if (auto __cond = dynamic_cast <IR::boolean_constant *> (__br->cond))
                __br->br[__cond->value]->stmt.front() = 
                    IR::create_unreachable();

        for(auto __next : __node->next) __self(__self,__next);
    };

    __dfs(__dfs,__entry);

    /* Remove newly generated unreachable blocks. */
    unreachable_remover __remover(__func,__entry);
}


void CFGsimplifier::compress_jump(IR::function *__func,node *__entry) {
    std::unordered_map <IR::block_stmt *,IR::block_stmt *> __map;
    __map.reserve(__func->stmt.size());

    auto &&__dfs = [&](auto &&__self,node *__node) -> void {
        auto *__block = __node->block;
        if (!__map.try_emplace(__block).second) return;
        auto __last = __block->stmt.back();
        while(dynamic_cast <IR::jump_stmt *> (__last)) {
            runtime_assert("WTF is that?",__node->next.size() == 1);
            auto __next = __node->next[0];
            /* Compress the jump. */
            if(__next->prev.size() == 1) {
                /* Move the commands from one block to another. */
                auto &__vec = __block->stmt;
                auto &__tmp = __next->block->stmt;
                __vec.pop_back();
                __vec.insert(__vec.end(),__tmp.begin(),__tmp.end());
                __last = __vec.back();

                /* Update the CFG graph. */
                __map[__next->block] = __block;
                __node->next = __next->next;
            } else return __self(__self,__next);
        }

        if(dynamic_cast <IR::branch_stmt *> (__last)) {
            runtime_assert("WTF is that?",__node->next.size() == 2);
            /* Visit down. */
            __self(__self,__node->next[0]);
            __self(__self,__node->next[1]);
        } else { /* No more unreachable now! */
            safe_cast <IR::return_stmt *> (__last);
            runtime_assert("WTF is that?",__node->next.size() == 0);
        }
    };

    __dfs(__dfs,__entry);
    for(auto __block : __func->stmt)
        for(auto __phi : __block->get_phi_block()) 
            for(auto &__data : __phi->cond) {
                auto __block = __map[__data.label];
                if (__block) __data.label = __block;
            }

    /* Just remove those unreachable branches. */
    std::vector <IR::block_stmt *> __vec;
    for(auto __block : __func->stmt)
        if(!__map[__block]) __vec.push_back(__block);
    __func->stmt = std::move(__vec);
}



void CFGsimplifier::remove_single_phi(IR::function *__func,node *__entry) {
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

    { /* Remove useless phi. */
        std::vector <IR::node *> __vec;
        for(auto __block : __func->stmt) {
            size_t __cnt = 0;
            for(auto __phi : __block->get_phi_block())
                if(use_map.count(__phi->dest))
                    __vec.push_back(__phi);
                else ++__cnt; /* Need to be removed. */
            auto __beg = __block->stmt.cbegin();
            __block->stmt.erase(__beg,__beg + __cnt);
            /* Copy the useful data back. */
            memcpy(__block->stmt.data(),__vec.data(),__vec.size() * sizeof(IR::node *));
            __vec.clear();
        }
    }

}


}