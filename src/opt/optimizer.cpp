#include "mem2reg.h"
#include "deadcode.h"

namespace dark::OPT {

/* The real function that controls all the optimization. */
void SSAbuilder::try_optimize(IR::function *__func) {
    /* This builds up SSA form and lay phi statement. */
    dominate_maker __maker {create_node(__func->stmt.front())};
    deadcode_eliminator __eliminator {__func,__maker};

}

}