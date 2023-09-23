#include "ASMallocator.h"
#include <algorithm>

namespace dark::ASM {

/* Never spill a short-life usage.*/
double ASMlifeanalyser::usage_info::distance_factor
    (int __blk) const noexcept {
    return static_cast <double> (life_length) + 1e-8;
}
/* Callee save: No cost. */
double ASMlifeanalyser::usage_info::get_callee_weight
    (int __blk) const noexcept { return 0.00000; }
/* Caller save: Temporaries. */
double ASMlifeanalyser::usage_info::get_caller_weight
    (int __blk) const noexcept  {
    return call_weight / distance_factor(__blk);
}
/* Spilled: Use/Def count as weight.  */
double ASMlifeanalyser::usage_info::get_spill_weight
    (int __blk) const noexcept {
    return use_weight / distance_factor(__blk);
}


/**
 * Core allocator.
 * 
 * We take a complex yet effective method:
 * 
 * When allocating a register, we will analyze the
 * usage of the register first.
 * 
 * We split the status of a register into 3 sorts:
 *      1. Callee save: Saved registers. No cost.
 *      2. Caller save: Temporaries. (Cost in crossing function.)
 *      3. Spilled: Cost in every use/def.
 * 
 * Besides, as there are holes in life ranges, so we may reassign
 * a allocated register as much as possible.
 * 
 * We will note down the lifetime holes of a certain register.
 * When the complete life range of one virtual register
 * can be covered by the holes of another register in
 * one block (this cannot be ignore!) , we will consider
 * reassign this register.
 * 
 * After working out the possible register allocation set,
 * we will have all these possibilities:
 *      I. No extra-spill case.
 *          1. Assign a caller-save register. (From life-hole).
 *          2. Assign a callee-save register. (From life-hole).
 *          3. Assign a caller-save register. (From avail-list).
 *          4. Assign a callee-save register. (From avail-list).
 *      II. Extra-spill case.
 *          1. Spill current virtual register.
 *          2. Assign a caller-save register. (From spill).
 *          3. Assign a callee-save register. (From spill).
 * 
 * We may evaluate the cost of all these possibilities,
 * and choose the best one (of course).
 * 
 * Specially, when a caller-save and a callee-save register
 * are equally good, we will choose the caller-save one.
 * 
 * 
*/
ASMallocator::ASMallocator(function *__func) {
    return;
    __impl = new ASMlifeanalyser {__func};

    block_pos.reserve(__func->blocks.size() + 1);
    block_pos.push_back(0);
    for(auto __block : __func->blocks)
        block_pos.push_back(__block->get_impl_val <int> ());

    [this]() -> void {
        auto __beg = __impl->usage_map.begin();
        auto __end = __impl->usage_map.end();
        work_list.reserve(__impl->usage_map.size());
        while(__beg != __end)
            work_list.push_back(__beg++);

        /* Sort in ascending order of beginning. */
        std::sort(work_list.begin(), work_list.end(),
            [this] (pair_pointer __lhs, pair_pointer __rhs) -> bool {
                return __lhs->second.get_begin() < __rhs->second.get_begin();
            });
    } ();

    for(auto __ptr : work_list) {
        expire_old(__ptr->second.get_begin());
        linear_allocate(__ptr);
    }
}


void ASMallocator::linear_allocate(pair_pointer __ptr) {
    auto &&[__vir, __info] = *__ptr;
    /* Special optimize for those short lived. */
    if (__info.life_length == 0) {
        vir_map[__vir] = address_info {address_info::TEMPORARY,0u};
        return;
    }

    auto __beg = __ptr->second.get_begin();
    auto __end = __ptr->second.get_end();
    auto [__callee,__caller] = find_best_suit(__beg,__end);

    if (__info.call_weight == 0) {
        /* Prefer to use callee always if possible. */
        if (__callee != -1) return use_callee(__ptr, __callee); 
        if (callee_pool.size())
            return use_callee(__ptr, callee_pool.allocate());

        if (__caller != -1) return use_caller(__ptr, __caller);
        if (caller_pool.size())
            return use_caller(__ptr, caller_pool.allocate());

        /* If same weight: spill callee first. */

        auto __spill_callee = get_callee_spill(__beg);
        auto __spill_caller = get_caller_spill(__beg);

        /* Compare spill caller , spill callee and spill self. */

        /* First compare spill caller and spill callee. */

        /* Then compare the best spill with spill self. */

    } else {
        /* Prefer to use caller, but callee is also accepteable. */
        if (__caller != -1) return use_caller(__ptr, __caller);
        if (caller_pool.size())
            return use_caller(__ptr, caller_pool.allocate());
        /* Whether or not to use depends on the value cost. */

        /* We will not spill a temporary for an unsuitable use! */
        auto __spill_caller = get_caller_spill(__beg);

        /* Compare spill caller , spill self and use temporary. */

        /* First compare spill caller and spill self. */


        /* Then compare the best spill with use temporary. */


    }
}


void ASMallocator::expire_old(int __count) {
    for(size_t i = 0 ; i < CALLEE_SIZE; ++i) {
        auto &__ref = active_list[i + 0];
        if (__ref.update(__count)) callee_pool.deallocate(i);
    }

    for(size_t i = 0 ; i < CALLER_SIZE; ++i) {
        auto &__ref = active_list[i + CALLEE_SIZE];
        if (__ref.update(__count)) caller_pool.deallocate(i);
    }
}


/* Tries to find a hole for caller/callee. */
std::pair <int,int> ASMallocator::find_best_suit(int __beg_pos,int __end_pos) {
    auto __end = std::lower_bound(block_pos.begin(), block_pos.end(), __end_pos);
    int __callee = -1, __caller = -1;

    /* In one block, find potential hole. */
    if (__beg_pos >= *(__end - 1)) {
        int __wasted;
        /* Find out the potential callee. */
        __wasted = INT32_MAX;
        for(size_t i = 0 ; i < CALLEE_SIZE; ++i) {
            auto &__ref = active_list[i + 0];
            auto  __use = __ref.next_use();
            if (__use < __end_pos) continue;
            /* Available case! */
            if (__use - __beg_pos < __wasted) {
                __wasted = __use - __beg_pos;
                __callee = i;
            }
        }
        __wasted = INT32_MAX;
        /* Find out the potential caller. */
        for(size_t i = 0 ; i < CALLER_SIZE; ++i) {
            auto &__ref = active_list[i + CALLEE_SIZE];
            auto  __use = __ref.next_use();
            if (__use < __end_pos) continue;
            /* Available case! */
            if (__use - __beg_pos < __wasted) {
                __wasted = __use - __beg_pos;
                __caller = i;
            }
        }
    }
    return {__callee, __caller};
}


void ASMallocator::use_callee(pair_pointer __ptr, unsigned __callee) {
    auto &&[__vir, __info] = *__ptr;
    auto &__ref = active_list[__callee + 0];
    __ref.merge_range(&__ptr->second);
    vir_map[__vir] = {address_info::CALLEE, __callee};
}


void ASMallocator::use_caller(pair_pointer __ptr, unsigned __caller) {
    auto &&[__vir, __info] = *__ptr;
    auto &__ref = active_list[__caller + CALLEE_SIZE];
    __ref.merge_range(&__ptr->second);
    vir_map[__vir] = {address_info::CALLER, __caller};
}


}