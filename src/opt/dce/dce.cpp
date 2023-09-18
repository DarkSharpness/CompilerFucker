#include "deadcode.h"


namespace dark::OPT {


/**
 * @brief A non-aggressive dead code eliminator.
 * This eliminator will not change the CFG structure.
 * It only removes all CFG unrelated and effectless code.
 * This is rather mild as it requires no CFG analysis nor
 * function call analysis.
 * 
 */
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

    if (__func->is_unreachable()) return;

    auto &&__rule = [](IR::node *__node) -> bool {
        if(dynamic_cast <IR::store_stmt *> (__node)) {
            return false;
        } else if(auto __call = dynamic_cast <IR::call_stmt *> (__node)) {
            auto __func = __call->func;
            /* In this pass, only builtin function without inout are safe. */
            return !__func->is_side_effective();
        } else if(dynamic_cast <IR::return_stmt *> (__node)) {
            return false;
        } else if(dynamic_cast <IR::jump_stmt *> (__node)) {
            return false;
        } else if(dynamic_cast <IR::branch_stmt *> (__node)) {
            return false;
        } else {
            return true;
        }
    };

    collect_useful(__func,__rule);
    replace_undefined();
    spread_useful();
    remove_useless(__func);
}


/**
 * @brief Just spread the useful data.
 * It will use data in worklist and then clear the worklist.
*/
void deadcode_eliminator::spread_useful() {
    while(!work_list.empty()) {
        auto __info = work_list.front(); work_list.pop();
        if (__info->removable) continue;
        for(auto *__use : __info->uses) {
            auto __prev = get_info(__use);
            if (__prev->removable) {
                __prev->removable = false;
                work_list.push(__prev);
            }
        }
    }
}


}