#pragma once

#include "optnode.h"
#include "mem2reg.h"
#include <stack>
#include <queue>
#include <unordered_set>
#include <unordered_map>

namespace dark::OPT {


struct DCE_base {
    struct info_holder {
        /* The inner data. */
        IR::node  *data = nullptr;
        OPT::node *node = nullptr;
        /* Whether the node is removable. */
        bool  removable = true;
        /* Just a cache of uses. */
        std::vector <IR::temporary *> uses;

        /* Init with a the definition node and all usage information. */
        void init(IR::node *__node,IR::block_stmt *__block) {
            /* Should only call once! */
            runtime_assert("Invalid SSA form!",data == nullptr);

            data = __node; 
            node = __block->get_impl_ptr <OPT::node> ();

            /* Init the data. */
            auto __vec = __node->get_use();
            uses.reserve(__vec.size());
            for(auto __use : __vec)
                if(auto __tmp = dynamic_cast <IR::temporary *> (__use))
                    uses.push_back(__tmp);
        }
    };

    /* Mapping from defs to uses and real node. */
    std::unordered_map <IR::temporary *, info_holder *> info_map;
    /* This is the real data holder. */
    std::deque <info_holder  > info_list;
    std::queue <info_holder *> work_list;

    /**
     * @brief Get the info tied to the temporary.
     * @param __temp The temporary (Non-nullptr)!
     * @return The info holder tied to the temporary.
     */
    info_holder *get_info(IR::temporary *__temp) {
        auto *&__ptr = info_map[__temp];
        if(!__ptr) __ptr = &info_list.emplace_back();
        return __ptr;
    }

    /**
     * @brief Create a new info holder for the temporary.
     * 
     * @param __temp The temporary defined by the node in SSA.
     * @return The info holder tied to the temporary.
     */
    info_holder *create_info(IR::node *__node) {
        auto *__def  = __node->get_def();
        return __def ? get_info(__def) : &info_list.emplace_back();
    }

    /* First, we will collect all def-use chains. */
    template <class _Func>
    void collect_useful(IR::function *__func,_Func &&__rule) {
        for(auto __block : __func->stmt) {
            /* No blocks are unreachable now. */
            for(auto __stmt : __block->stmt) {
                info_holder * __info = create_info(__stmt);
                __info->init(__stmt,__block);

                /* Tries to update removable according to rule. */
                __info->removable = __rule(__stmt);

                /* A side effective command! */
                if (!__info->removable) work_list.push(__info);
            }
        }
    }

    void replace_undefined();
    void remove_useless(IR::function *);
};


/**
 * @brief Eliminator for dead code in a function.
 * This is an unaggressive dead code eliminator.
 * 
 */
struct deadcode_eliminator : DCE_base {
    /* This is the list of variables to remove. */
    deadcode_eliminator(IR::function *,node *);
    void spread_useful();
};

/**
 * @brief Eliminator for aggressive dead code in a function.
 * It will analyze the potential effect in the reverse graph.
 * If a block's definition is not linked to any use, it is
 * dead. 
 * 
*/
struct aggressive_eliminator : DCE_base {
    std::unordered_set <node *> live_set;
    std::queue <node *> node_list;

    aggressive_eliminator(IR::function *,node *);
    void spread_live();
    void update_dead(IR::function *);
};



}