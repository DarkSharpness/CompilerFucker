#include "inline.h"
#include "IRinfo.h"

#include "mem2reg.h"

namespace dark::OPT {

/* Recollect size info. */
void recursive_inliner::rebuild_info(IR::function *__func) {
    auto * __info  = __func->get_impl_ptr <function_info> ();
    size_t __count = 0;
    __info->called_func.clear();
    for(auto __block : __func->stmt) {
        __count += __block->stmt.size();
        for(auto __stmt : __block->stmt) {
            auto __call = dynamic_cast <IR::call_stmt *> (__stmt);
            if (!__call) continue;
            __info->called_func.insert(__call->func);
        }
    }
    __info->statement_count = __count;
}


/**
 * @brief Recursive inline on DAG function graph. 
 */
recursive_inliner::recursive_inliner(
    IR::function *__func,node *__entry,_Pass &&__pass,void *__this) {
    rebuild_info(__func);
    while(try_inline(__func,__this)) {
        __pass(__func,__entry);
        rebuild_info(__func);
    }
    /* To ensure safety, rebuild after inlining. */
    rebuild_info(__func);
}

bool recursive_inliner::is_inlinable
    (function_info *__info,IR::block_stmt *__block,IR::function *__next) {
    if (__next->is_builtin) return false;
    auto __temp = __next->get_impl_ptr <function_info> ();
    /* Avoid dead inlining. */
    if (__temp->self_call() && ban_list[__next].count() > BAN_DEPTH)
        return false;

    size_t __N1 = __info->statement_count;
    size_t __N2 = __temp->statement_count;
    size_t __N3 = __info->func->stmt.size();
    size_t __N4 = __next->stmt.size();
    size_t __N5 = __block->stmt.size();

    if (__N2 > MAX_FUNC)  return false;
    if (__N3 > MAX_BLOCK || __N4 > MAX_BLOCK) return false;

    return ((__N1 < MIN_FUNC)  | (__N2 < MIN_FUNC)  | 
            (__N3 < MIN_BLOCK) | (__N4 < MIN_BLOCK) | __N5 < 4)
        || (__N1 + __N2 < SUM_FUNC && __N3 + __N4 < SUM_BLOCK);
}

bool recursive_inliner::try_inline(IR::function *__func,void *__this) {
    auto *__info = __func->get_impl_ptr <function_info> ();

    if (__info->called_func.empty()) return false;
    if (++depth > MAX_DEPTH)  return false;

    /* The new blocks appended. */

    std::unordered_map <IR::call_stmt *,inline_info> __map;
    std::vector <bool> __bit_map (__func->stmt.size());
    size_t __cnt = 0;
    for(auto __block : __func->stmt) {
        for(auto __stmt : __block->stmt) {
            if (__map.size() > MAX_COUNT) break;
            auto __call = dynamic_cast <IR::call_stmt *> (__stmt);
            if (!__call) continue;
            auto __next = __call->func;
            if (!is_inlinable(__info,__block,__next)) continue;
            auto &__ref = __map[__call] = (inline_visitor {
                __call, __next, __this
            });
            __bit_map[__cnt] = true;
        } ++__cnt;
    }

    if (!__map.size()) return false;
    std::vector <IR::block_stmt *> __new; __cnt = 0;

    auto &&__replace_phi = []
        (IR::block_stmt *__old,IR::block_stmt *__new,IR::block_stmt *__cur) {
        for(auto __phi : __cur->get_phi_block()) __phi->update(__old,__new);            
    };

    for(const auto __block : __func->stmt) {
        __new.push_back(__block);
        if (!__bit_map[__cnt++]) continue;

        /* Just work on the bak. */
        const auto __vec = __block->stmt;
        __block->stmt.clear();

        IR::block_stmt *__top = __block;
        for(auto __stmt : __vec) {
            auto &__ref = __top->stmt;
            __ref.push_back(__stmt);
            auto __call = dynamic_cast <IR::call_stmt *> (__stmt);
            if (!__call) continue;
            auto __iter = __map.find(__call);
            if (__iter == __map.end()) continue;

            __ref.pop_back();
            auto *__entry = __iter->second.entry;
            auto &__tmp   = __entry->stmt;
            __ref.insert(__ref.end(),__tmp.begin(),__tmp.end());

            /* May involves phi replacement. */
            if (auto *__br = dynamic_cast <IR::branch_stmt *> (__ref.back())) {
                __replace_phi(__entry,__top,__br->br[0]);
                __replace_phi(__entry,__top,__br->br[1]);
            } else {
                auto *__jump = safe_cast <IR::jump_stmt *> (__ref.back());
                __replace_phi(__entry,__top,__jump->dest);
            }

            auto &__list  = __iter->second.list;
            /* Insert a range (except the entry & exit). */
            __new.insert(__new.end(),__list.begin() + 1,__list.end());
            __top = __iter->second.exit;
        }

        auto &__ref = __top->stmt;
        /* May involves phi replacement. */
        if (auto *__br = dynamic_cast <IR::branch_stmt *> (__ref.back())) {
            __replace_phi(__block,__top,__br->br[0]);
            __replace_phi(__block,__top,__br->br[1]);
        } else if (auto *__jump = dynamic_cast <IR::jump_stmt *> (__ref.back())){
            __replace_phi(__block,__top,__jump->dest);
        }
    }

    /* Set the ban_list */
    const size_t __pos = depth - 1;
    for(auto &&[__call,__dat] : __map) ban_list[__call->func].set(__pos);

    auto *__SSA = static_cast <SSAbuilder *> (__this);
    for(auto __block : __new) __SSA->reset_CFG(__block);
    for(auto __block : __new) __SSA->visitBlock(__block);


    __new.swap(__func->stmt);
    return true;
}




}