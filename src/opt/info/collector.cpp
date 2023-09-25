#include "collector.h"
#include "info_debug.h"

#include <queue>
#include <unordered_map>
#include <unordered_set>

namespace dark::OPT {


/**
 * @brief Build all information of the function.
 * Use it after all function info has been collected.
 */
void info_collector::build(_Info_Container &__array) {
    for (auto &__info : __array) function_graph::tarjan(__info);
    if (function_graph::work_topo(__array)) {

    }
    function_graph::resolve_leak(__array);
    function_graph::resolve_used(__array);

    // for (auto &__info : __array) print_info(__info);
}


/**
 * @brief A function information collector.
 * It will not do any analysis or change to the function.
 * It only collect all usage information that will be used in the optimization.
 * This is the first time of visiting the function.
 * 
 */
info_collector::info_collector(function_info &__info) {
    auto &use_map = __info.use_map;

    /* First collect basic def/use information and inout information. */
    [&]() ->void {
        /* Update the global usage map~ */
        auto &&__update_global = [&__use_map = __info.used_global_var]
            (IR::non_literal *__var,uint8_t __state) -> void {
            auto __global = dynamic_cast <IR::global_variable *> (__var);
            if (!__global) return;
            std::string_view __view = __global->name;
            if (__view.substr(0,5) == "@str.") return;
            __use_map[__global] |= __state;
        };

        for(auto __block : __info.func->stmt) {
            __info.statement_count += __block->stmt.size();
            for(auto __stmt : __block->stmt) {
                if (auto __load = dynamic_cast <IR::load_stmt *> (__stmt))
                    __update_global(__load->src ,function_info::LOAD);
                else if (auto __store = dynamic_cast <IR::store_stmt *> (__stmt)) {
                    __update_global(__store->dst,function_info::STORE);
                    __info.store_name.insert(__store->dst->type.name());
                    __info.func->exist_store = true;    /* Has store. */
                } else if (auto __call  = dynamic_cast <IR::call_stmt *> (__stmt)) {
                    /* Collect the inout information. */
                    auto __called = __call->func;
                    __info.called_func.insert(__called);
                    __info.func->inout_state |= __called->inout_state;
                }

                for(auto __use : __stmt->get_use())
                    if (auto *__var = dynamic_cast <IR::non_literal *> (__use))
                        use_map.try_emplace(__var);

                auto __def = __stmt->get_def();
                if (__def) use_map[__def].def_node = __stmt;
            }
        }

        /* Init all reliance. */
        for(auto &&[__,__vec] : use_map) __vec.set_impl_ptr(new reliance);
    }();

    /* Leak information initialization. */
    [&]() -> void {
        auto &&__update = [&](auto *__tmp,bool __state) -> reliance * {
            auto *__var = dynamic_cast <IR::non_literal *> (__tmp);
            if (!__var) return nullptr; /* Failed to cast, so return. */
            auto &__vec = use_map[__var];
            auto *__ptr = __vec.get_impl_ptr <reliance> ();
            __ptr->used_flag |= __ptr->USED_BIT;
            __ptr->leak_flag |= __state;
            return __ptr;
        };

        for(auto __block : __info.func->stmt) {
            for(auto __stmt : __block->stmt) {
                if (auto __call = dynamic_cast <IR::call_stmt *> (__stmt)) {
                    auto __func = __call->func; /* The function called. */
                    size_t __n = __call->args.size();
                    for(size_t i = 0 ; i < __n ; ++i) {
                        auto *__var = dynamic_cast <IR::non_literal *> (__call->args[i]);
                        if (!__var) continue;

                        auto &__vec = use_map[__var];
                        auto *__ptr = __vec.get_impl_ptr <reliance> ();
                        if (__n > reliance::THRESHOLD)
                            __ptr->leak_flag |= __ptr->USED_BIT;
                        else if(__func->is_builtin) {
                            __ptr->used_flag |= __ptr->USED_BIT;
                        } else { /* Leak and used now rely on function now. */
                            __ptr->leak_flag |= __ptr->FUNC_BIT;
                            __ptr->used_flag |= __ptr->FUNC_BIT;
                            __ptr->leak_func[__func].set(i);
                            __ptr->used_func[__func].set(i);
                        }
                    }
                } else if(auto __ret = dynamic_cast <IR::return_stmt *> (__stmt)) {
                    __update(__ret->rval,true);
                } else if(auto __store = dynamic_cast <IR::store_stmt *> (__stmt)) {
                    __update(__store->src,true);
                    __update(__store->dst,false);
                } else if(auto __br = dynamic_cast <IR::branch_stmt *> (__stmt)) {
                    __update(__br->cond,false);
                }
            }
        }
    }();

    [&]() {
        std::queue <IR::node *> work_list;

        /* Collect all leaks. */
        for(auto &&[__var,__vec] : use_map) {
            auto *__ptr = __vec.get_impl_ptr <reliance> ();
            if (__ptr->leak_flag) work_list.push(__vec.def_node);
        }

        /* Work out the leak information. */
        while(!work_list.empty()) {
            auto *__node = work_list.front(); work_list.pop();
            /* Will not spread the leak info case. */
            if (!dynamic_cast <IR::phi_stmt *> (__node)) continue;

            auto *__ptr = use_map[__node->get_def()].get_impl_ptr <reliance> ();
            for(auto __use : __node->get_use()) {
                if (auto *__var = dynamic_cast <IR::non_literal *> (__use)) {
                    auto &__vec = use_map[__var];
                    auto *__tmp = __vec.get_impl_ptr <reliance> ();
                    if (__tmp->merge_leak(__ptr)) work_list.push(__vec.def_node);
                }
            }
        }

        /* Collect all uses. */
        for(auto &&[__var,__vec] : use_map) {
            auto *__ptr = __vec.get_impl_ptr <reliance> ();
            if (__ptr->used_flag) work_list.push(__vec.def_node);
        }

        /* Work out the usage information. */
        while(!work_list.empty()) {
            auto *__node = work_list.front(); work_list.pop();
            /* Will not spread the used info case. */
            if (dynamic_cast <IR::call_stmt *> (__node) || !__node) continue;

            auto *__ptr = use_map[__node->get_def()].get_impl_ptr <reliance> ();
            for(auto __use : __node->get_use()) {
                if (auto *__var = dynamic_cast <IR::non_literal *> (__use)) {
                    auto &__vec = use_map[__var];
                    auto *__tmp = __vec.get_impl_ptr <reliance> ();
                    if (__tmp->merge_used(__ptr)) work_list.push(__vec.def_node);
                }
            }
        }
    }();

    /* Work out the arg_state. */
    [&]() -> void {
        for(auto *__arg : __info.func->args) {
            auto &__vec = use_map[__arg];
            auto *__ptr = __vec.get_impl_ptr <reliance> ();
            if (!__ptr) {
                __vec.set_impl_ptr(new reliance);
                __arg->used_flag = __arg->leak_flag = 0b00;
            } else {
                __arg->used_flag = __ptr->used_flag;
                __arg->leak_flag = __ptr->leak_flag;
            }
        }
    }();
}


/**
 * @brief Use tarjan to build up function call map.
 * This implement is effient and effective using tarjan.
 * 
 */
void function_graph::tarjan(function_info &__info) {
    std::vector <function_info *> __stack;
    /* So-called tarjan algorithm. */
    auto &&__dfs = [&](auto &&__self,function_info &__info) -> void {
        __stack.push_back(&__info);
        __info.dfn = __info.low = dfn_count++;

        for(auto *__func : __info.called_func) {
            if (__func->is_builtin) continue;
            auto &__next = *__func->get_impl_ptr <function_info> ();
            __next.caller_func.insert(__info.func);
            if (__next.dfn == __next.NPOS) {
                __self(__self,__next);
                __info.low = std::min(__info.low,__next.low);
            } else if (__next.scc == __next.NPOS) /* In stack. */
                __info.low = std::min(__info.low,__next.dfn);
        }

        if (__info.dfn == __info.low) {
            while(__stack.back() != &__info) {
                auto *__temp = __stack.back();
                __stack.pop_back();
                __temp->scc = scc_count;
            }
            __stack.pop_back();
            __info.scc = scc_count++;
        }
    };
    /* If not visited, work out the SCC. */
    if(__info.dfn == __info.NPOS) __dfs(__dfs,__info);
    runtime_assert("Tarjan algorithm failed.",__stack.empty());
}


/**
 * @brief Build the topological graph after tarjan.
 * This function will not touche the inner data.
 */
void function_graph::build_graph(const _Info_Container &__array) {
    edge_in.clear();
    degree_out.clear();
    edge_set.clear();
    edge_in.resize(scc_count);
    degree_out.resize(scc_count);

    /* Find the real info holder. */
    for(auto &__info : __array) {
        /* Tries to locate the main function. */
        if (!__init_pair.main && __info.func->name == "main")
            __init_pair.main = const_cast <function_info *> (&__info);

        for(auto *__func : __info.called_func) {
            if (__func->is_builtin) continue;
            auto &__next = *__func->get_impl_ptr <function_info> ();

            /* Tries to find global_init function. */
            if (!__init_pair.init && __func->name == ".__global__init__") {
                __init_pair.init = &__next;
                continue;
            }

            size_t __id1 = __info.scc;
            size_t __id2 = __next.scc;
            /* No duplicated edge or self edge. */
            if (!edge_set.insert({__id1,__id2}).second || __id1 == __id2) 
                continue;
            ++degree_out[__id1];
            edge_in[__id2].push_back(__id1);
        }
    }
}


/**
 * @brief After tarjan, use the DAG to help build data within.
 * @param __array Array of real data inside.
 */
bool function_graph::work_topo(_Info_Container &__array) {
    build_graph(__array);

    /* The real info holder of a scc. */
    std::vector <function_info *> real_info(scc_count);
    for(auto &__info : __array)
        if (__info.dfn == __info.low)
            real_info[__info.scc] = &__info;

    /**
     * ----------------------------------------------
     * Prelude to the core function.
     * Build up the DAG graph and merge within SCC.
     * ----------------------------------------------
    */

    /* Work out the real_info within one SCC. */
    for(auto &__info : __array) {
        auto *__root = __info.real_info = real_info[__info.scc];
        __root->merge_within_SCC(__info);
    }

    /**
     * ----------------------------------------------
     * Core part of the function as below.
     * Using the DAG to work out every real_info.
     * ----------------------------------------------
    */

    std::vector <size_t> work_list; /* Stack-like worklist. */
    for(size_t i = 0 ; i < scc_count; ++i)
        if(!degree_out[i]) work_list.push_back(i);
    while(!work_list.empty()) {
        size_t __n = work_list.back(); work_list.pop_back();
        auto &__root = *real_info[__n];
        for(auto __m : edge_in[__n]) {
            if (--degree_out[__m] == 0) work_list.push_back(__m);
            real_info[__m]->merge_between_SCC(__root);
        }
    }

    /**
     * ----------------------------------------------
     * Final part of the function as below.
     * Remove self from recursive call if not called.
     * ----------------------------------------------
    */

    for(size_t i = 0 ; i < scc_count; ++i)
        if(!edge_set.count({i,i})) {
            auto *__info = real_info[i];
            __info->recursive_func.erase(__info->func);
        }

    /**
     * Specially, deal with the case of main-function calling .__global_init__
     * If no-input, we can work out the state of the init in theory.
     * Therefore, we can remove the call to the init function.
     * Specially, those array init cannot be replaced.
     * If global init only calls builtin non-IO function, we can remove the call!
     * We may force inline it and remove all non-array init.
     * For array init, we may not replace it.
     * 
    */
    if (!__init_pair.init) return false;
    auto __init = __init_pair.init;
    if (__init->recursive_func.empty() && !__init_pair.init->func->inout_state) {
        /* Try to simulate and replace all global variable load/store with local ones. */
        return true;
    } else {
        /* Merge it normally. */
        runtime_assert("Global init cannot be recursive.",
            !__init->self_call(),
            __init_pair.main->real_info == __init_pair.main
        );
        __init_pair.main->merge_between_SCC(*__init);
        return false;
    }
}


/**
 * @brief Resolve the dependency of the function.
 * @param __array Resolve the dependency.
 */
void function_graph::resolve_leak(_Info_Container &__array) {
    std::unordered_map <
        IR::function_argument *,
        std::vector <IR::function_argument*>
    > __map;
    std::vector <IR::function_argument *> work_list;

    for(auto &__info : __array) {
        size_t __n  = __info.func->args.size();
        if (__n > reliance::THRESHOLD) continue;
        for(auto __arg : __info.func->args) {
            work_list.push_back(__arg);
            if (!__arg->may_leak()) continue;
            __arg->leak_flag &= 0b01;

            /* Build the mapping from one argument to another. */
            auto *__ptr = __info.use_map.at(__arg).get_impl_ptr <reliance> ();
            for(auto [__func,__bits] : __ptr->leak_func) {
                auto *__args = __func->args.data();
                size_t __beg = __bits._Find_first();
                while(__beg != __bits.size()) {
                    __map[__args[__beg]].push_back(__arg);
                    __beg = __bits._Find_next(__beg);
                }
            }
        }
    }

    while(!work_list.empty()) {
        auto *__arg = work_list.back(); work_list.pop_back();
        if (!__arg->leak_flag) continue;
        auto __iter = __map.find(__arg);
        if (__iter == __map.end()) continue;

        for(auto *__dep : __iter->second) {
            auto  __tmp = __dep->leak_flag | __arg->leak_flag;
            if (__tmp  != __dep->leak_flag) {
                __dep->leak_flag = __tmp;
                work_list.push_back(__dep);
            }
        }
    }

    /* Update all usage information. */
    for(auto &__info : __array) {
        for(auto &&[__var,__vec] : __info.use_map) {
            auto *__ptr = __vec.get_impl_ptr <reliance> ();
            if (!__ptr->may_leak()) continue;
            __ptr->leak_flag &= 0b01;

            for(auto [__func,__bits] : __ptr->leak_func) {
                auto *__args = __func->args.data();
                size_t __beg = __bits._Find_first();
                while(__beg != __bits.size()) {
                    __ptr->leak_flag |= __args[__beg]->leak_flag;
                    __beg = __bits._Find_next(__beg);
                } if (__ptr->leak_flag) break;
            }
        }
    }
}


/**
 * @brief Resolve the dependency of the function.
 * @param __array Resolve the dependency.
 */
void function_graph::resolve_used(_Info_Container &__array) {
    std::unordered_map <
        IR::function_argument *,
        std::vector <IR::function_argument*>
    > __map;
    std::vector <IR::function_argument *> work_list;

    for(auto &__info : __array) {
        size_t __n  = __info.func->args.size();
        if (__n > reliance::THRESHOLD) continue;
        for(auto __arg : __info.func->args) {
            work_list.push_back(__arg);
            if (!__arg->may_used()) continue;
            __arg->used_flag &= 0b01;

            /* Build the mapping from one argument to another. */
            auto *__ptr = __info.use_map.at(__arg).get_impl_ptr <reliance> ();
            for(auto [__func,__bits] : __ptr->used_func) {
                auto *__args = __func->args.data();
                size_t __beg = __bits._Find_first();
                while(__beg != __bits.size()) {
                    __map[__args[__beg]].push_back(__arg);
                    __beg = __bits._Find_next(__beg);
                }
            }
        }
    }

    while(!work_list.empty()) {
        auto *__arg = work_list.back(); work_list.pop_back();
        if (!__arg->used_flag) continue;
        auto __iter = __map.find(__arg);
        if (__iter == __map.end()) continue;

        for(auto *__dep : __iter->second) {
            auto  __tmp = __dep->used_flag | __arg->used_flag;
            if (__tmp  != __dep->used_flag) {
                __dep->used_flag = __tmp;
                work_list.push_back(__dep);
            }
        }
    }

    /* Update all usage information. */
    for(auto &__info : __array) {
        for(auto &&[__var,__vec] : __info.use_map) {
            auto *__ptr = __vec.get_impl_ptr <reliance> ();
            if (!__ptr->may_used()) continue;
            __ptr->used_flag &= 0b01;

            for(auto [__func,__bits] : __ptr->used_func) {
                auto *__args = __func->args.data();
                size_t __beg = __bits._Find_first();
                while(__beg != __bits.size()) {
                    __ptr->used_flag |= __args[__beg]->used_flag;
                    __beg = __bits._Find_next(__beg);
                } if (__ptr->used_flag) break;
            }
        }
    }
}


/**
 * @brief Make up the order of function call graph.
 * It will determine the order of inlining.
 * 
 * @param __array The array of function info.
 * @return The order of function call graph.
 */
std::vector <IR::function *> function_graph::inline_order(_Info_Container &__array) {
    /* Scc count of the function. */
    build_graph(__array);

    std::vector <std::vector <IR::function *>> scc_list(scc_count);

    for(auto &__info : __array) scc_list[__info.scc].push_back(__info.func);

    std::vector <size_t> work_list; /* Stack-like worklist. */
    for(size_t i = 0 ; i < scc_count; ++i)
        if(!degree_out[i]) work_list.push_back(i);

    std::vector <IR::function *> __order; __order.reserve(__array.size());

    while (!work_list.empty()) {
        size_t __n  = work_list.back();
        work_list.pop_back();
        /* Add to the back of the vector. */
        auto &__vec = scc_list[__n];
        __order.insert(__order.end(),__vec.begin(),__vec.end());

        auto *__info = __vec.front()->get_impl_ptr <function_info> ()->real_info;
        for(auto __m : edge_in[__n])
            if (--degree_out[__m] == 0) work_list.push_back(__m);
    }

    /* Complete the call graph. */
    

    return __order;
}


}