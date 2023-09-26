#include "mem2reg.h"
#include "dominate.h"
#include "deadcode.h"
#include "cfg.h"
#include "sccp.h"

#include "peephole.h"
#include "collector.h"
#include "inline.h"


namespace dark::OPT {


/**
 * @brief This container must satisfy the condition that
 * all its poiners won't be invalidated after push_back.
 * 
 * Naturally, std::deque or std::list is a good choice.
 * 
 */
using _Info_Container = std::deque <function_info>;

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
    _Info_Container info_list;
    /* A variable holding optimization state. */
    const auto __optimize_state = optimize_options::get_state();

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
        info_collector {info_list.emplace_back(&__func)};
    }

    /* Spread the data recursively! */
    info_collector::build(info_list);

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

    auto &&__checker = [] (IR::function *__func) -> void {
        for(auto __block : __func->stmt) {
            auto *__node = __block->get_impl_ptr <node> ();
            if (__node->next.size() > 2) throw error("haha");
        }
    };

    /* A mild inline pass. */
    auto &&__inline_pass = [__optimize_state,__checker]
        (IR::function *__func,node *__entry) -> void {
        unreachable_remover {__func,__entry};
        deadcode_eliminator {__func,__entry};
        if (__optimize_state.enable_SCCP) {
            constant_propagatior {__func,__entry};
            branch_cutter        {__func,__entry};
            deadcode_eliminator  {__func,__entry};
        }
        if (__optimize_state.enable_CFG) {
            branch_compressor   {__func,__entry};
            deadcode_eliminator {__func,__entry};
        }
        unreachable_remover {__func,__entry};
        if (__optimize_state.enable_PEEP) {
            local_optimizer     {__func,__entry};
            constant_propagatior{__func,__entry};
            branch_cutter       {__func,__entry};
            deadcode_eliminator {__func,__entry};
        }
    };

    /* Work out the inline order. */
    if (__optimize_state.enable_INLINE) {
        std::vector <IR::function*> __order = function_graph::inline_order(info_list);
        for(auto *__func : __order) {
            auto *__entry = create_node(__func->stmt.front());
            recursive_inliner {__func,__entry,__inline_pass,this};    
            __checker(__func);
            __inline_pass(__func,__entry);
        }
    }

    auto &&__replace_undefined = [](IR::function *__func) -> void {
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
        __replace_undefined(&__func);
    }
}


void SSAbuilder::update_pool(std::vector <IR::initialization> &global_variables) {
    auto &__range = constant_calculator::generated;
    for(auto &__var : __range) global_variables.push_back({&__var,__var.const_val});
}

void SSAbuilder::reset_CFG(IR::block_stmt *__block) {
    auto *__node = create_node(__block);
    __block->set_impl_ptr(__node);
    __node->prev.clear();
    __node->next.clear();
    __node->dom.clear();
    __node->fro.clear();
}


void SSAbuilder::reset_CFG(IR::function *__func) {
    for(auto __block : __func->stmt) reset_CFG(__block);
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



}