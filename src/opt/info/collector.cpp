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
        for(auto __block : __info.func->stmt) {
            for(auto __stmt : __block->stmt) {
                for(auto __use : __stmt->get_use())
                    if (auto __var = dynamic_cast <IR::non_literal *> (__use))
                        use_map[__var].push_back(__stmt);
                auto __def = __stmt->get_def();
                if (__def) use_map[__def].def_node = __stmt;

                /* Collect the inout information. */
                if (auto __call  = dynamic_cast <IR::call_stmt *> (__stmt)) {
                    auto __called = __call->func;
                    if (!__info.called_func.insert(__called).second) continue;
                    __info.func->inout_state |= __called->inout_state;
                }
            }
        }
    }();


    /* Next, collect some necessary global information. */
    [&]() -> void {
        for(auto &&[__var,__vec] : use_map) {
            __vec.set_impl_ptr(new reliance);
            if (auto __global = dynamic_cast <IR::global_variable *> (__var)) {
                auto &__ref = __info.used_global_var[__global];
                for(auto __node : __vec) {
                    if (dynamic_cast <IR::load_stmt *> (__node))
                        __ref |= function_info::LOAD;
                    else if (dynamic_cast <IR::store_stmt *> (__node))
                        __ref |= function_info::STORE;
                    if (__ref == function_info::BOTH) break;
                }
            }
        }
    }();


    /* Leak information initialization. */
    [&]() -> void {
        std::queue <IR::node *> work_list;
        auto &&__update = [&](auto *__tmp,uint8_t __state) -> reliance * {
            auto *__var = dynamic_cast <IR::non_literal *> (__tmp);
            if (!__var) return nullptr; /* Failed to cast, so return. */
            auto &__vec = use_map[__var];
            work_list.push(__vec.def_node);
            auto *__ptr = __vec.get_impl_ptr <reliance> ();
            __ptr->rely_flag = __state;
            return __ptr;
        };

        for(auto __block : __info.func->stmt) {
            for(auto __stmt : __block->stmt) {
                if (auto __call = dynamic_cast <IR::call_stmt *> (__stmt)) {
                    auto __func = __call->func;
                    size_t __n = __call->args.size();
                    for(size_t i = 0 ; i < __n ; ++i) {
                        auto *__var =
                            dynamic_cast <IR::non_literal *> (__call->args[i]);
                        if (!__var) continue;

                        auto &__vec = use_map[__var];
                        work_list.push(__vec.def_node);

                        auto *__ptr = __vec.get_impl_ptr <reliance> ();
                        if (__n > reliance::THRESHOLD)
                            __ptr->rely_flag = IR::function_argument::LEAK;
                        else if(__func->is_builtin) {
                            __ptr->rely_flag = IR::function_argument::USED;
                        } else { /* Rely on function now. */
                            __ptr->rely_flag = IR::function_argument::FUNC;
                            __ptr->rely_func[__func].set(i);
                        }
                    }
                } else if(auto __ret = dynamic_cast <IR::return_stmt *> (__stmt)) {
                    __update(__ret->rval,IR::function_argument::LEAK);
                } else if(auto __store = dynamic_cast <IR::store_stmt *> (__stmt)) {
                    __update(__store->src,IR::function_argument::LEAK);
                    __update(__store->dst,IR::function_argument::USED);
                } else if(auto __br = dynamic_cast <IR::branch_stmt *> (__stmt)) {
                    __update(__br->cond,IR::function_argument::USED);
                }
            }
        }

        /* Work out the leak information. */
        while(!work_list.empty()) {
            auto *__node = work_list.front(); work_list.pop();
            /* Will not spread the leak info case. */
            if (dynamic_cast <IR::call_stmt *> (__node)
            ||  dynamic_cast <IR::load_stmt *> (__node) || !__node) continue;
            auto *__ptr = use_map[__node->get_def()].get_impl_ptr <reliance> ();
            for(auto __use : __node->get_use()) {
                if (auto *__var = dynamic_cast <IR::non_literal *> (__use)) {
                    auto &__vec = use_map[__var];
                    auto *__tmp = __vec.get_impl_ptr <reliance> ();
                    if (__tmp->merge_leak(__ptr)) work_list.push(__vec.def_node);
                }
            }
        }
    }();



    /* Work out the arg_state. */
    [&]() -> void {
        std::queue <IR::node *> work_list;
        for(auto &&[__var,__vec] : use_map) {
            auto *__ptr = __vec.get_impl_ptr <reliance> ();
            if (__ptr->is_used()) work_list.push(__vec.def_node);
        }

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

        for(auto *__arg : __info.func->args) {
            auto &__ref = __arg->state;
            auto &__vec = use_map[__arg];
            /* If not used, of course safe. */
            if (__vec.empty()) { __ref = IR::function_argument::DEAD; continue; }

            /* If only used in function as dead argument. */
            __ref = __vec.get_impl_ptr <reliance> ()->rely_flag;
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
void function_graph::work_topo(std::deque <function_info> &__array) {
    /* Do nothing. */
    if (!scc_count) return;

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

    /* Find the real info holder. */
    for(auto &__info : __array) {
        if (__info.dfn == __info.low)
            real_info[__info.scc] = &__info;
        for(auto *__func : __info.called_func) {
            if (__func->is_builtin) continue;
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



}



/**
 * @brief Resolve the depency of the function.
 * @param __array 
 */
void function_graph::resolve_dependency(std::deque <function_info> &__array) {
    std::unordered_map <
        IR::function_argument *,
        std::vector <IR::function_argument*>
    > __map;
    std::vector <IR::function_argument *> work_list;

    for(auto &__info : __array) {
        size_t __n  = __info.func->args.size();
        if (__n > reliance::THRESHOLD) continue;
        for(auto __arg : __info.func->args) {
            if (__arg->state != IR::function_argument::DEAD)
                work_list.push_back(__arg);
            if (!(__arg->state & IR::function_argument::FUNC)) continue;
            __arg->state ^= IR::function_argument::FUNC;

            auto *__ptr = __info.use_map.at(__arg).get_impl_ptr <reliance> ();
            for(auto [__func,__bits] : __ptr->rely_func) {
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
        auto __iter = __map.find(__arg);
        if (__iter == __map.end()) continue;
        for(auto *__dep : __iter->second) {
            auto  __tmp = __dep->state | __arg->state;
            if (__tmp  != __dep->state) {
                __dep->state = __tmp;
                work_list.push_back(__dep);
            }
        }
    }
}

}