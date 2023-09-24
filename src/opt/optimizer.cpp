#include "mem2reg.h"
#include "dominate.h"
#include "deadcode.h"
#include "cfg.h"
#include "sccp.h"

#include "peephole.h"
#include "collector.h"
#include "inline.h"


namespace dark::OPT {

void build_virtual_exit(IR::function *__func,node *__exit) {
    for(auto __block : __func->stmt) {
        auto *__last = __block->stmt.back();
        if (!dynamic_cast <IR::return_stmt*> (__last)) continue;
        auto __next = __block->get_impl_ptr <node> ();
        __exit->next.push_back(__next);
        __next->prev.push_back(__exit);
    }
}

/* The real function that controls all the optimization. */
void SSAbuilder::try_optimize(std::vector <IR::function>  &global_functions) {
    /* Function information list. */
    std::deque  <function_info> info_list;
    /* A variable holding optimization state. */
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

        if (__optimize_state.enable_SCCP) {
            constant_propagatior{&__func,__entry};
            branch_cutter       {&__func,__entry};
            deadcode_eliminator {&__func,__entry};
        }

        if (__optimize_state.enable_CFG) {
            tail_recursion_pass {&__func,this};
            branch_compressor   {&__func,__entry};
            deadcode_eliminator {&__func,__entry};
            branch_compressor   {&__func,__entry};
            deadcode_eliminator {&__func,__entry};
        }

        unreachable_remover {&__func,__entry};

        /* Peephole optimization. */
        if(__optimize_state.enable_PEEP) {
            local_optimizer     {&__func,__entry};
            deadcode_eliminator {&__func,__entry};   
            local_optimizer     {&__func,__entry};
            constant_propagatior{&__func,__entry};
            branch_cutter       {&__func,__entry};
            deadcode_eliminator {&__func,__entry};
        }

        /* Aggressive deadcode elemination. */
        if(__optimize_state.enable_DCE) {
            node __exit{nullptr};
            reverse_CFG(&__func);
            /* Now, all exit have no prev. */
            build_virtual_exit(&__func,&__exit);
            dominate_maker {&__exit,true};
            reverse_CFG(&__func);
            aggressive_eliminator(&__func,__entry);
            rebuild_CFG(&__func);
            unreachable_remover(&__func,__entry);
        }

        /* Final simplification after DCE and PEEP. */
        if (__optimize_state.enable_CFG) {
            tail_recursion_pass {&__func,this};
            branch_compressor   {&__func,__entry};
            deadcode_eliminator {&__func,__entry};
        }

        /* After first pass of optimization, collect information! */
        info_collector {info_list.emplace_back(&__func),std::false_type{}};

    }

    /* Spread the data recursively! */
    for (auto &__info : info_list) function_graph::tarjan(__info);
    if (function_graph::work_topo(info_list)) {
        std::cerr << "TODO!!\n";
    }
    function_graph::resolve_dependency(info_list);
    // for (auto &__info : info_list) print_info(__info);

    /* Second pass: using collected information to optimize. */
    for(auto &__func : global_functions) {
        auto *__entry = create_node(__func.stmt.front());
        dead_argument_eliminater {&__func,__entry};
        deadcode_eliminator      {&__func,__entry};

        if (__optimize_state.enable_CFG) {
            tail_recursion_pass {&__func,this};
            branch_compressor   {&__func,__entry};
            deadcode_eliminator {&__func,__entry};
            branch_compressor   {&__func,__entry};
            deadcode_eliminator {&__func,__entry};
        }

        unreachable_remover {&__func,__entry};

        /* Peephole optimization. */
        if(__optimize_state.enable_PEEP) {
            local_optimizer     {&__func,__entry};
            deadcode_eliminator {&__func,__entry};   
            local_optimizer     {&__func,__entry};
            constant_propagatior{&__func,__entry};
            branch_cutter       {&__func,__entry};
            deadcode_eliminator {&__func,__entry};
        }
    }

    auto __replace_undefined = [](IR::function *__func) {
        IR::undefined *__undef[3] = {
            IR::create_undefined({},1),
            IR::create_undefined({},2),
            IR::create_undefined({},3)
        };
        IR::definition *__new[3] = {
            IR::create_pointer(0),
            IR::create_integer(0),
            IR::create_boolean(0),
        };

        for(auto __block : __func->stmt) {
            for(auto __stmt : __block->stmt) {
                for(size_t i = 0 ; i < 3 ; ++i)
                    __stmt->update(__undef[i],__new[i]);
            }
        }
    };

    /* Final pass: replace all undefined. */
    for(auto &__func : global_functions) {
        malloc_eliminator {&__func,nullptr};
        __replace_undefined(&__func);
    }


}


void SSAbuilder::update_pool(std::vector <IR::initialization> &global_variables) {
    auto &__range = constant_calculator::generated;
    for(auto &__var : __range) global_variables.push_back({&__var,__var.const_val});
}


void SSAbuilder::reset_CFG(IR::function *__func) {
    for(auto __block : __func->stmt) {
        auto *__node = create_node(__block);
        __block->set_impl_ptr(__node);
        __node->prev.clear();
        __node->next.clear();
        __node->dom.clear();
        __node->fro.clear();
    }
}

void SSAbuilder::rebuild_CFG(IR::function *__func) {
    reset_CFG(__func);
    visitFunction(__func);
}

void SSAbuilder::reverse_CFG(IR::function *__func) {
    for(auto __block : __func->stmt) {
        auto *__node = __block->get_impl_ptr <node> ();
        std::swap(__node->prev,__node->next);
    }
}


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
            default: __os << "??";
        } __os << '\n';
    }
    __os << "Global variable:\n ";
    for(auto [__var,__state] : __info.used_global_var) {
        __os << "    " << __var->data() << " : ";
        switch(__state) {
            case function_info::LOAD : __os << "Load only!"; break;
            case function_info::STORE: __os << "Store only!"; break;
            case function_info::BOTH : __os << "Load & Store!"; break;
            default: __os << "??";
        } __os << '\n';
    }
    __os << "Local temporary:\n";
    for(auto &[__var,__use] : __info.use_map) {
        __os << __var->data() << " : ";
        auto __ptr = __use.get_impl_ptr <reliance> ();
        switch(__ptr->rely_flag) {
            case IR::function_argument::DEAD : __os << "Not used!";  break;
            case IR::function_argument::USED : __os << "Just used!"; break;
            case IR::function_argument::LEAK : __os << "Leaked!";    break;
            default: __os << "??";
        } __os << '\n';

    }

    const char *__msg[4] = { "NONE","IN ONLY","OUT ONLY","IN AND OUT" };
    __os << "Inout state: " << __msg[__info.func->inout_state];
    __os << "\n----------------------\n";
}



}