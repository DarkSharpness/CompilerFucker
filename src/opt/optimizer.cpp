#include "mem2reg.h"
#include "dominate.h"
#include "deadcode.h"
#include "cfg.h"

namespace dark::OPT {

/* The real function that controls all the optimization. */
void SSAbuilder::try_optimize(std::vector <IR::function>  &global_functions) {
    auto __optimize_state = optimize_options::get_state();
    for(auto &__func : global_functions) {
        auto *__entry = create_node(__func.stmt.front());
 
        /* This removes all unreachable branches in advance. */
        unreachable_remover __remover {&__func,__entry};

        /* This builds up SSA form and lay phi statement. */
        dominate_maker __maker {&__func,__entry};

        /* Eliminate dead code. */
        if(__optimize_state.enable_DCE)
            deadcode_eliminator __eliminator {&__func,__entry};

        /* Simplify the CFG. */
        if(__optimize_state.enable_CFG)
            CFGsimplifier __simplifier {&__func,__entry};

    }


}

}