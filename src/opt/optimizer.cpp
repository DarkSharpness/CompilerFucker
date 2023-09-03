#include "mem2reg.h"
#include "dominate.h"
#include "deadcode.h"
#include "cfg.h"

namespace dark::OPT {

/* The real function that controls all the optimization. */
void SSAbuilder::try_optimize(std::vector <IR::function>  &global_functions) {
    for(auto &__func : global_functions) {
        auto *__entry = create_node(__func.stmt.front());
 
        /* This removes all unreachable branches in advance. */
        unreachable_remover __remover {&__func,__entry};

        /* This builds up SSA form and lay phi statement. */
        dominate_maker __maker {&__func,__entry};

        /* Eliminate dead code. */
        if(optimize_options::DCE_enabled())
            deadcode_eliminator __eliminator {&__func,__entry};

    }


}

}