#pragma once

#include "optnode.h"
#include <queue>
#include <unordered_set>
#include <unordered_map>

namespace dark::OPT {

/* Return a new jump statement to replace a branch. */
inline IR::jump_stmt *replace_branch
    (IR::branch_stmt *__br,IR::block_stmt *__dest) {
    delete __br; /* First release the memory. */
    auto *__jump = new IR::jump_stmt;
    __jump->dest = __dest;
    return __jump;
}


/* This is helper class that removes all useless nodes. */
struct unreachable_remover {
    std::unordered_set <IR::block_stmt *> block_set;
    std::queue <node *> work_list;
    unreachable_remover(IR::function *,node *);

    /**
     * @brief Add all nodes that satisfies the condition to work_list.
     * @tparam _Func Condition function type.
     * @param __node The node that is being visited.
     * @param __func Condition function.
     */
    template <class _Func>
    void dfs(node *__node,_Func &&__func) {
        /* All ready visited. */
        if(block_set.insert(__node->block).second == false) return;
        if(__func(__node)) work_list.push(__node);
        for(auto __next : __node->next) dfs(__next,std::forward <_Func> (__func));
    }

    /* Spread those unreachable block tags. */
    void spread_unreachable();
    /**
     * @brief Update branch jump from unreachable branches.
     * Note that this may result in single phi! (e.g: %1 = phi[1,%entry])
    */
    void update_phi_source();
    /* Remove all unreachable blocks. */
    void remove_unreachable(IR::function *);

};

/**
 * @brief This is helper class that will simplify CFG.
 * It will remove unreachable branches and replace all single phi.
 * It will will also compress useless jumps.
 * 
*/
struct graph_simplifier {
    /* Map of temporary usage. */
    std::unordered_map <IR::temporary *,std::vector <IR::node *>> use_map;

    /* A list of phi statements to remove. */
    std::queue <IR::phi_stmt *> work_list;

    /* Return the phi statement type. */
    static IR::definition *get_phi_type(IR::phi_stmt *__phi) {
        IR::temporary  *__def = __phi->dest;
        IR::definition *__tmp = nullptr;

        for(auto [__use,__] : __phi->cond) {
            if(__use == __def) continue;
            if(__tmp == nullptr) __tmp = __use;
            /* Multiple definition: not a zero/ single phi! */
            else if(__tmp != __use) return nullptr;
        }

        return __tmp ? __tmp : IR::create_undefined(__def->type);
    }

    graph_simplifier(IR::function *,node *);

    /* Replace constant branch with jump and set another branch unreachable. */
    void replace_const_branch(IR::function *,node *);
    /* Remove a single source phi statement. */
    void remove_single_phi(IR::function *,node *);
    /* Compress multiple jump to one (e.g. BB1 -> ... -> BBn  || BB1 -> BBn) . */
    void compress_jump(IR::function *,node *);

};


}