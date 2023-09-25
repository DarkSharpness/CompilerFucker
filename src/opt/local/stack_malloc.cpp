#include "peephole.h"
#include "IRinfo.h"

namespace dark::OPT {


malloc_eliminator::malloc_eliminator(IR::function *__func,node *) {
    auto *__info = __func->get_impl_ptr <function_info> ();
    if (!__info) return;

    constexpr size_t THRESHOLD = 256;
    size_t __sum = 0;

    std::unordered_map <IR::temporary *,IR::local_variable *> __remap;
    std::vector <IR::call_stmt *> __call_list;

    for(auto __block : __func->stmt) {
        for(auto __node : __block->stmt) {
            auto *__call = dynamic_cast <IR::call_stmt *> (__node);
            if (!__call || __call->func->name != "malloc") continue;
            auto *__arg = dynamic_cast <IR::integer_constant *> (__call->args[0]);
            if (!__arg) { warning("What happened?"); return; }
            auto __rely = __info->use_map.at(__call->dest).get_impl_ptr <reliance> ();
            /* Replace a malloc with alloca. */
            __remap.emplace(__call->dest,nullptr);
            __sum += __arg->value;
            __call_list.push_back(__call);
        }
    }

    if (__sum > THRESHOLD || __call_list.empty()) return;

    size_t __top = 0;
    for(auto &[__tmp,__var] : __remap) {
        __var       = new IR::local_variable;
        __var->name = "%salloc." + std::to_string(__top++);
        __var->type = __tmp->type;
    }

    __top = 0;
    for(auto __block : __func->stmt) {
        for(auto &__node : __block->stmt) {
            if (__call_list[__top] != __node) {
                for(auto __use : __node->get_use()) {
                    auto __tmp = dynamic_cast <IR::temporary *> (__use);
                    if (!__tmp) continue;
                    auto __iter = __remap.find(__tmp);
                    if (__iter == __remap.end()) continue;
                    __node->update(__tmp,__iter->second);
                } continue;
            }
            auto *__stmt = new IR::allocate_stmt;
            __stmt->dest = __remap[__call_list[__top++]->dest];
            __node = __stmt;
        }
    }

}


}