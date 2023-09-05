#include "mem2reg.h"
#include "dominate.h"
#include "deadcode.h"
#include "cfg.h"
#include "sccp.h"

namespace dark::OPT {


/* The real function that controls all the optimization. */
void SSAbuilder::try_optimize(std::vector <IR::function>  &global_functions) {
    auto __optimize_state = optimize_options::get_state();
    /* First pass : simple optimization. */
    for(auto &__func : global_functions) {
        auto *__entry = create_node(__func.stmt.front());

        /* This removes all unreachable branches in advance. */
        unreachable_remover __remover {&__func,__entry};
        if(__func.is_unreachable()) continue;

        /* This builds up SSA form and lay phi statement. */
        dominate_maker __maker {&__func,__entry};

        /* Eliminate dead code. */
        if(__optimize_state.enable_DCE)
            deadcode_eliminator __eliminator {&__func,__entry};

        /* Sparse constant propagation. */
        if(__optimize_state.enable_SCCP)
            constant_propagatior __propagatior {&__func,__entry};

        /* Simplify the CFG. */
        if(__optimize_state.enable_CFG)
            graph_simplifier __simplifier {&__func,__entry};

        { /* After simplification, the CFG graph may go invalid. */
            __entry = rebuild_CFG(&__func);
            unreachable_remover __remover {&__func,__entry};
        }
    }

    for(auto &__func : global_functions) {
        // second pass.
    }

}

node *SSAbuilder::rebuild_CFG(IR::function *__func) {
    node *__entry = nullptr;
    for(auto __block : __func->stmt) {
        auto *__node = create_node(__block);
        if(!__entry) __entry = __node;
        __node->prev.clear();
        __node->next.clear();
        __node->dom.clear();
        __node->fro.clear();
    } /* Rebuild the CFG! */
    visitFunction(__func);
    return __entry;
}

}