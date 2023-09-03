#include "deadcode.h"


namespace dark::OPT {

deadcode_eliminator::deadcode_eliminator
    (IR::function *__func,dominate_maker &__maker) {
    /**
     * Firstly, collect all the usage information. (First def, then use)
     * 
     * For those "def"less variables outside phi, it will be replaced with 0.
     * For those "def"less variables in phi, we will remove them.
     * For those undefined branch value in phi, we will set them as below.
     * (
     *  e.g:
     *      %x = phi i32 [1,%entry],[%undef,%bb1]
     *  it will change into:
     *      %x = phi i32 [1,%entry]      // Actually invalid in llvm IR.
     * )
     * Value that jump from these directions will be undefined.
     * Undefined value may be used in constant propagation.
     * 
     * 
     * 
     * Next, mark all variables with potential-side-effect.(Store/Call).
     * 
     * These potential-side-effects may also spread to other variables.
     * Specially, for branch statement, we should find out whether it
     * may bring side-effects (like jump to a side-effective block.),
     * and then remove side-effectless branches.
     * 
     * 
     * 
     * Finally, remove all "use"less and side-effectless definitions. 
     * (e.g. %a = call may_have_side_fx() . %a cannot be removed. )
     * 
     * Note that "use"less info might spread from one variable to another. 
     * (e.g. %a = %b + 1. if %a "use"less and %b only used once (here),
     * then %b is also "use"less, which may be removed later).
     * 
     *
     * 
     * Now, all dead code have been eliminated. And we can further
     * optimize the IR using other methods like constant propagation.
    */

    // /* First, work out the potential side effect of each node. */
    // spread_side_fx(__maker.node_rpo.front());


    /* First, we will collect all def-use chains. */
    for(auto __block : __func->stmt) {
        /* No blocks are unreachable now. */
        for(auto __stmt : __block->stmt) {
            info_holder * __info = create_info(__stmt);
            std::vector <info_holder *> __vec;
            for(auto *__use : __stmt->get_use())
                if(auto __temp = dynamic_cast <IR::temporary *> (__use))
                    __vec.push_back(get_info(__temp));
            __info->init(__stmt,__vec);

            /* A side effective command! */
            if(!__info->removable) work_list.push(__info);
        }
    }

    /* Spread the useful data. */
    while(!work_list.empty()) {
        auto __info = work_list.front(); work_list.pop();
        for(auto *__use : __info->data->get_use())
            if (auto __temp = dynamic_cast <IR::temporary *> (__use)) {
                auto __prev = get_info(__temp);
                if (__prev->removable) {
                    __prev->removable = false;
                    work_list.push(__prev);
                }
            }
    }

    { /* Remove useless data. */
        std::unordered_set <IR::node *> __set;
        for(auto &__info : info_list)
            if(__info.removable) __set.insert(__info.data);
        std::vector <IR::node *> __vec;
        for(auto __block : __func->stmt) {
            __vec.clear();
            for(auto __stmt : __block->stmt)
                if(!__set.count(__stmt))
                    __vec.push_back(__stmt);
            __block->stmt.swap(__vec);
        }
    }

}


// void deadcode_eliminator::spread_side_fx(node *__entry) {
//     /* The set that contains all node. */
//     std::unordered_set <node *> node_set;
// 
//     /* dfs to make these side_fxs. */
//     auto &&__dfs = [&](node *__node,auto &&__self) {
//         if(!node_set.insert(__node).second) return;
// 
//         side_effect_t __fx = FX_NONE;
//         for(auto __stmt : __node->block->stmt)
//             __fx |= get_side_fx(__stmt);
// 
//         for(auto *__next : __node->next) {
//             __self(__next,__self);
//             /* First easy iteration. */
//             __fx |= __next->side_fx;
//         }
// 
//         __node->side_fx = __fx;
//     };
//     __dfs(__entry,__dfs);
// 
//     std::queue <node *> work_list;
//     for(auto __iter : node_set) work_list.push(__iter);
//     node_set.clear(); /* No longer useful now, clear memory storage. */
// 
//     while(!work_list.empty()) {
//         auto __node = work_list.front(); work_list.pop();
// 
//         /* Will never be changed! */
//         if(__node->side_fx == FX_FULL) continue;
// 
//         /* Iterate this node awa. */
//         side_effect_t __fx = __node->side_fx;
//         for(auto *__next : __node->next) __fx |= __next->side_fx;
// 
//         /* If changed, add prev to worklist. */
//         if(__fx == __node->side_fx) continue;
//         for(auto *__prev : __node->prev) work_list.push(__prev);
//     }
// 
// }




}