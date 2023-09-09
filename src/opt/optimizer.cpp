#include "mem2reg.h"
#include "dominate.h"
#include "deadcode.h"
#include "cfg.h"
#include "sccp.h"

#include "peephole.h"
#include "collector.h"
#include "inline.h"


namespace dark::OPT {

/* This function is used to print the information of the function. */
void print_info(const function_info &__info,std::ostream &__os = std::cerr) {
    __os << "----------------------\n";
    __os << "Caller: " << __info.func->name << "\nCallee:";
    for (auto __callee : __info.real_info->recursive_func)
        __os << " " << __callee->name;
    __os << "\nArgument state:\n";
    auto __iter = __info.func->args.begin();
    for (auto __arg : __info.func->args) {
        __os << "    " << __arg->data() << " : ";
        switch(__arg->state) {
            case IR::function_argument::DEAD: __os << "Not used!"; break;
            case IR::function_argument::USED: __os << "Just used!"; break;
            case IR::function_argument::LEAK: __os << "Leaked!"; break;
            case IR::function_argument::FUNC: __os << "Func depend: ";
            auto *__ptr = __info.use_map.at(__arg).get_impl_ptr <leak_info>();
            if (!__ptr) runtime_assert(":(");
            for (auto [__func,__bits] : __ptr->leak_func) {
                __os << __func->name << "[";
                bool __first = true;
                for(size_t i = 0 ; i != __bits.size() ; ++i)
                    if(__bits[i]) {
                        if(!__first) __os << ',';
                        else  __first = false;
                        __os << i;
                    }
                __os << "] ";
            }
        } __os << '\n';
    }
    __os << "----------------------\n";
}


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
            single_killer       {&__func,__entry};
            branch_compressor   {&__func,__entry};
            deadcode_eliminator {&__func,__entry};
        }

        unreachable_remover {&__func,__entry};

        /* Peephole optimization. */
        if(__optimize_state.enable_PEEP) {
            local_optimizer     {&__func,__entry};
            deadcode_eliminator {&__func,__entry};
        }

        /* After first pass of optimization, collect information! */
        info_collector {info_list.emplace_back(&__func),std::false_type{}};

        std::cerr << __func.name << " finished!\n";
    }

    /* Spread the data recursively! */
    for (auto &__info : info_list) function_graph::tarjan(__info);
    function_graph::work_topo(info_list);
    function_graph::resolve_dependency(info_list);
    for (auto &__info : info_list) print_info(__info);

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
    visitFunction(__func); return __entry;
}


}