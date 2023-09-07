#include "mem2reg.h"
#include "dominate.h"
#include "deadcode.h"
#include "cfg.h"
#include "sccp.h"

namespace dark::OPT {


/* The real function that controls all the optimization. */
void SSAbuilder::try_optimize(std::vector <IR::function>  &global_functions) {
    const auto __optimize_state = optimize_options::get_state();
    /* First pass : simple optimization. */
    for(auto &__func : global_functions) {
        auto *__entry = create_node(__func.stmt.front());

        /* This removes all unreachable branches in advance. */
        unreachable_remover{&__func,__entry};
        if(__func.is_unreachable()) continue;

        /* This builds up SSA form and lay phi statement. */
        dominate_maker{&__func,__entry};

        /* Eliminate dead code safely. */
        deadcode_eliminator{&__func,__entry};
        std::cerr << 0 << '\n';

        if (__optimize_state.enable_SCCP) {
            constant_propagatior{&__func,__entry};
            branch_cutter       {&__func,__entry};
            deadcode_eliminator {&__func,__entry};            
        }

        if (__optimize_state.enable_CFG) {
            single_killer       {&__func,__entry};
            branch_compressor   {&__func,__entry};
            deadcode_eliminator {&__func,__entry};
        }

        unreachable_remover {&__func,__entry};

        /* Peephole optimization. */
        if(__optimize_state.enable_PEEP) {
        }

        /* After first pass of optimization, collect information! */


        std::cerr << __func.name << " finished!\n";
    }

    /* Second pass: using collected information to optimize. */
    for(auto &__func : global_functions) {
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