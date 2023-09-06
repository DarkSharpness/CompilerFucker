#include "deadcode.h"


namespace dark::OPT {

deadcode_eliminator::deadcode_eliminator(IR::function *__func,node *) {
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
            __info->init(__stmt);
            /* A side effective command! */
            if(!__info->removable) work_list.push(__info);
        }
    }

    /* Specially, we will replace all undefed variables as undefined. */
    for(auto &__info : info_list) {
        for(auto &__use : __info.uses) {
            if(!info_map.count(__use)) {
                // std::cerr << "Undefed temporary: " << __use->data() << '\n';
                __info.data->update(__use,IR::create_undefined(__use->type));
            }
        }
    }

    /* Spread the useful data. */
    while(!work_list.empty()) {
        auto __info = work_list.front(); work_list.pop();
        for(auto *__use : __info->uses) {
            auto __prev = get_info(__use);
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
            for(auto __stmt : __block->stmt)
                if(!__set.count(__stmt))
                    __vec.push_back(__stmt);
            __block->stmt.swap(__vec);
            __vec.clear();
        }
    }

}


}