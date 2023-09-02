#include "mem2reg.h"
#include "deadcode.h"
#include "cfg.h"

namespace dark::OPT {

/* The real function that controls all the optimization. */
void SSAbuilder::try_optimize(IR::function *__func) {
    /* This removes all unreachable branches. */
    /* This builds up SSA form and lay phi statement. */
    auto *__entry = create_node(__func->stmt.front()); 
    unreachable_remover __remover {__func,__entry};
    dominate_maker __maker {__entry};
    deadcode_eliminator __eliminator {__func,__maker};
}

}