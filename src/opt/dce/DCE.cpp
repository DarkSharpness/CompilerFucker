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
     * Then, for those unreachable blocks, we will first spread them in CFG,
     * then cut them all for further optimizing the CFG. 
     * (e.g. branch to a unreachable block will be changed into jump.)
     * (e.g. phi from a unreachable block will be removed.)
     * (e.g. jump to a unreachable block will make the block unreachable.)
     * (e.g. edge from a unreachable block will be removed.)
     * (e.g. block with no in-edge/out-edge (except ret) will be removed.)
     * 
     * 
     * 
     * Next, mark all variables with potential-side-effect.(Store/Call).
     * 
     * These potential-side-effect may also spread to other variables.
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
     * Now, all dead code have been eliminated. And we can further optimize
     * the IR using other methods like constant propagation.
    */

    for(auto __block : __func->stmt) {
        /* We won't visit unreachable blocks! */
        if(__block->is_unreachable()) continue;
        for(auto __stmt : __block->stmt) {
            auto __uses = __stmt->get_use();
            info_holder * __info = create_info(__stmt);
            std::vector <info_holder *> __vec;
            __vec.reserve(__uses.size());
            for(auto *__use : __uses)
                if(auto __temp = dynamic_cast <IR::temporary *> (__use))
                    __vec.push_back(get_info(__temp));
            __info->init(__stmt,__vec);
        }
    }
    

}

}