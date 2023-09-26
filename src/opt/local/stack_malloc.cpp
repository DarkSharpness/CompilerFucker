#include "peephole.h"
#include "IRinfo.h"

namespace dark::OPT {

void malloc_eliminator::find_leak(IR::function *__func) {
    std::vector <IR::temporary *> work_list;

    for(auto __block : __func->stmt) {
        for(auto __node : __block->stmt) {
            auto *__def = __node->get_def();
            if (__def) use_map[__def].def_node = __node;

            if (auto __store = dynamic_cast <IR::store_stmt *> (__node)) {
                if (auto __tmp = dynamic_cast <IR::temporary *> (__store->src)) {
                    use_map[__tmp].is_leak = true;
                    work_list.push_back(__tmp);
                } continue;
            }

            if (auto __ret = dynamic_cast <IR::return_stmt *> (__node)) {
                if (auto __tmp = dynamic_cast <IR::temporary *> (__ret->rval)) {
                    use_map[__tmp].is_leak = true;
                    work_list.push_back(__tmp);
                } continue;
            }

            auto *__call = dynamic_cast <IR::call_stmt *> (__node);
            if (!__call || __call->func->is_builtin) continue;

            /* For non-builtins, the argument might leak. */
            auto *__ptr = __call->func->args.data();
            for(size_t i = 0 ; i < __call->args.size() ; ++i) {
                auto __arg = __call->args[i];
                if (auto __tmp = dynamic_cast <IR::temporary *> (__arg);
                    __tmp != nullptr && __ptr[i]->is_leak()) {
                    use_map[__tmp].is_leak = true;
                    work_list.push_back(__tmp);
                }
            }
        }
    }

    while(!work_list.empty()) {
        auto *__tmp = work_list.back(); work_list.pop_back();
        auto __node = use_map[__tmp].def_node;

        /* Only phi_stmt may spread the leakage info currently. */
        auto __phi  = dynamic_cast <IR::phi_stmt *> (__node);
        if ( __phi == nullptr ) continue;

        for(auto [__val,__blk] : __phi->cond) {
            auto *__tmp = dynamic_cast <IR::temporary *> (__val);
            if (!__tmp) continue;
            auto &__ref = use_map[__tmp].is_leak;
            if (__ref == false) {
                __ref = true;
                work_list.push_back(__tmp);
            }
        }
    }
}


void malloc_eliminator::mark_malloc(IR::function *__func) {
    for(auto __block : __func->stmt) {
        for(auto __node : __block->stmt) {
            auto __call = dynamic_cast <IR::call_stmt *> (__node);
            /* Only non-leak malloc can be replaced with alloca. */
            if (!__call || __call->func->name != "malloc" || 
                use_map[__call->dest].is_leak) continue;
            call_set.insert(__call);
            mem_map.try_emplace(__call->dest);
        }
    }
}

/**
 * The optimization is done in two steps:
 * First we find all the malloc calls that are not leaked.
 * Then we replace them with alloca and update the use-def chain.
 * 
 * This may be helpful since malloc is a very expensive operation.
 * Note that this operation may require much more stack space.
 * 
 * If stack overflow, disable this optimization!
 * 
 * To be more aggressive, we may perform scalar replacement on these
 * allocated memory, but this is not implemented yet.
 * 
*/
malloc_eliminator::malloc_eliminator(IR::function *__func,node *) {
    auto *__info = __func->get_impl_ptr <function_info> ();
    if (!__info) return;

    find_leak(__func);
    mark_malloc(__func);

    size_t __top = 0;
    size_t __sum = 0;
    for(auto &[__tmp,__var] : mem_map) {
        __var       = new IR::local_variable;
        __var->name = "%salloc." + std::to_string(__top++);
        __var->type = __tmp->type;
        __sum      += (--__tmp->type).size();
    }

    /* Too big malloc replacement, so do nothing! */
    if (__sum > THRESHOLD) return;

    for(auto __block : __func->stmt) {
        for(auto &__node : __block->stmt) {
            auto *__call = dynamic_cast <IR::call_stmt *> (__node);
            if (!call_set.count(__call)) {
                for(auto __use : __node->get_use()) {
                    auto __tmp = dynamic_cast <IR::temporary *> (__use);
                    if (!__tmp) continue;
                    auto __iter = mem_map.find(__tmp);
                    if (__iter == mem_map.end()) continue;
                    __node->update(__tmp,__iter->second);
                } continue;
            }
            auto *__stmt = new IR::allocate_stmt;
            __stmt->dest = mem_map[__call->dest];
            __node = __stmt;
        }
    }

}


}