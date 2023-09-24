#include "collector.h"

#include <queue>
#include <unordered_map>
#include <unordered_set>

namespace dark::OPT {


/**
 * @brief A function information collector.
 * It will not do any analysis or change to the function.
 * It only collect all usage information that will be used in the optimization.
 * This is the first time of visiting the function.
 * 
 */
template <>
info_collector::info_collector <0> (function_info &__info,std::false_type) {
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
 * @brief A function information collector.
 * It will not do any analysis or change to the function.
 * This is the second time of visiting the function.
 * It will spread the information collected in the first time.
 * 
 */
template <>
info_collector::info_collector <1> (function_info &__info,std::true_type) {
    /* First, resolve the dependency. */
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
 * @brief After tarjan, use the DAG to help build data within.
 * @param __array Array of real data inside.
 */
bool function_graph::work_topo(std::deque <function_info> &__array) {
    /* Do nothing. */
    if (!scc_count) return false;

    /* Since there won't be too many functions, this is not a bad hash. */
    struct my_hash {
        size_t operator () (std::pair <size_t,size_t> __pair)
        const noexcept {
            return __pair.first << 32 | __pair.second;
        }
    };

    /* The real info holder of a scc. */
    std::vector <function_info *>      real_info(scc_count);
    std::vector <std::vector <size_t>> edge_in  (scc_count);
    std::vector <size_t>             degree_out (scc_count);
    std::unordered_set <std::pair <size_t,size_t>,my_hash> edge_set;


    /**
     * ----------------------------------------------
     * Prelude to the core function.
     * Build up the DAG graph and merge within SCC.
     * ----------------------------------------------
    */

    std::pair <function_info *,IR::function *> __init_pair = {{},{}};

    /* Find the real info holder. */
    for(auto &__info : __array) {
        if (__info.dfn == __info.low)
            real_info[__info.scc] = &__info;
        for(auto *__func : __info.called_func) {
            if (__func->is_builtin) continue;
            /* Global initer. */
            if (__func->name == ".__global__init__") {
                __init_pair = {&__info,__func}; continue;
            }
            auto &__next = *__func->get_impl_ptr <function_info> ();
            size_t __id1 = __info.scc;
            size_t __id2 = __next.scc;
            /* No duplicated edge or self edge. */
            if (!edge_set.insert({__id1,__id2}).second || __id1 == __id2) 
                continue;
            ++degree_out[__id1];
            edge_in[__id2].push_back(__id1);
        }
    }

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
    if (!__init_pair.second) return false;
    auto __init = __init_pair.second->get_impl_ptr <function_info> ();
    if (__init->recursive_func.empty() && !__init_pair.second->inout_state) {
        /* Try to simulate and replace all global variable load/store with local ones. */
        return true;
    } else {
        /* Merge it normally. */
        runtime_assert("Global init cannot be recursive.",!__init->self_call());
        __init_pair.first->merge_between_SCC(*__init);
        return false;
    }

}



/**
 * @brief Resolve the dependency of the function.
 * @param __array Resolve the dependency.
 */
void function_graph::resolve_leak(std::deque <function_info> &__array) {
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
void function_graph::resolve_used(std::deque <function_info> &__array) {
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


}