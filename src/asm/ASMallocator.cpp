#include "ASMallocator.h"
#include <algorithm>

namespace dark::ASM {

/* Never spill a short-life usage.*/
double ASMlifeanalyser::usage_info::distance_factor
    (int) const noexcept {
    return static_cast <double> (life_length) + 1e-8;
}
/* Callee save: Saved (No cost). */
double ASMlifeanalyser::usage_info::get_callee_weight
    (int) const noexcept { return 0.0; }
/* Caller save: Temporaries. */
double ASMlifeanalyser::usage_info::get_caller_weight
    (int) const noexcept  {
    return call_weight / distance_factor(0);
}
/* Spilled: Use/Def count as weight.  */
double ASMlifeanalyser::usage_info::get_spill_weight
    (int) const noexcept {
    return use_weight / distance_factor(0);
}


/* If the slot is empty, return INT32_MAX - 1*/
int ASMallocator::register_slot::next_use() const noexcept {
    return live_range.empty() ? NPOS : live_range.front().first;
}
/* Merge range to the front. */
void ASMallocator::register_slot::merge_range(pair_pointer __use) {
    life_map.push_back(__use);
    auto &&__range = __use->second.live_set;
    runtime_assert("This shouldn't happen!",
        !__range.empty() || __range.back().second > next_use()
    );

    auto __beg = __range.rbegin();
    auto __end = __range.rend();
    while(__beg != __end) live_range.push_front(*__beg++);
}
/* Return inner spill-cost. */
double ASMallocator::register_slot::spill_cost(bool is_caller) const noexcept {
    double __cost = 0;
    double __span = 1e-8;
    for(auto __ptr : life_map) {
        __cost += __ptr->second.use_weight;
        if (is_caller) __cost -= __ptr->second.call_weight;
        __span += __ptr->second.life_length;
    }
    return __cost / __span;
}
/* Update to test whether it is newly expired. */
bool ASMallocator::register_slot::update(const int __n) noexcept {
    if (live_range.empty()) return false;

    /* Update life map first. */
    auto __beg = life_map.begin();
    auto __end = life_map.end();
    const auto __bak = __beg;    
    while(__beg != __end) {
        if ((*__beg)->second.get_end() <= __n)
            *__beg = *--__end;
        else ++__beg;
    } life_map.resize(__end - __bak);

    /* Nothing is left alive...... */
    if (__end == __bak) { live_range.clear(); return true; }
    else do {
        auto &__front = live_range.front();
        /* If no exceeding the end, do nothing. */
        if (__front.second > __n) {
            __front.first = std::max(__front.first,__n);
            return false;
        } else live_range.pop_front();
    } while(true);
}


/* Whether a range is available for the slot. */
bool ASMallocator::stack_slot::contains(int __beg,int __end) const noexcept {
    if (auto __iter = --live_range.upper_bound({__beg,INT32_MAX});
        __iter->second > __beg) return false;

    if (auto __iter = live_range.upper_bound({__beg,-1});
        __iter->first < __end ) return false;

    return true;
}
/* Insert a range that won't conflict current range. */
void ASMallocator::stack_slot::merge_range(pair_pointer __use) {
    for(auto [__beg,__end] : __use->second.live_set)
        auto __iter = live_range.insert({__beg,__end}).first;
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
ASMallocator::ASMallocator(function *__func) : top_func(__func) {
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

    paint_color(__func);
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
    auto [__callee,__caller] = find_best_pair(__beg,__end);

    if (__info.call_weight == 0) {
        /* Prefer to use caller always if possible. */
        if (__caller != -1) return use_caller(__ptr, __caller);
        if (__callee != -1) return use_callee(__ptr, __callee);

        auto __spill_callee = get_callee_spill(0);
        auto __spill_caller = get_caller_spill(0);
        auto __current      = __ptr->second.get_spill_weight(0);

        /* Spill current variable.  */
        if (__current < __spill_callee.first &&
            __current < __spill_caller.first)
            return use_stack(__ptr,find_best_stack(__ptr));

        /* Spill the cheaper one. */
        if (__spill_callee.first < __spill_caller.first)
            return use_callee(__ptr,spill_callee(__spill_callee.second));
        else 
            return use_caller(__ptr,spill_caller(__spill_caller.second));

    } else {
        /* Prefer to use callee, but caller is also accepteable. */
        if (__callee != -1) return use_callee(__ptr, __callee);

        /* We will not spill a temporary for an unsuitable use! */
        auto __spill_callee = get_callee_spill(0);
        auto __current      = __ptr->second.get_spill_weight(0);

        /* Only when the use is cheap enough, will we choose caller. */
        if (__caller != -1) {
            auto __use_caller = __ptr->second.get_caller_weight(0);
            /* If too expensive to spill. */
            if (__use_caller < __current &&
                __use_caller < __spill_callee.first)
                return use_caller(__ptr, __caller);
        }

        /* Spill the cheaper one. */
        if (__current < __spill_callee.first)
            return use_stack(__ptr,find_best_stack(__ptr));
        else
            return use_callee(__ptr,spill_callee(__spill_callee.second));
    }
}


void ASMallocator::expire_old(int __count) {
    /* Expire callee save. */
    for(size_t i = 0 ; i < CALLEE_SIZE; ++i) {
        auto &__ref = active_list[i + 0];
        __ref.update(__count);
    }
    /* Expire caller save. */
    for(size_t i = 0 ; i < CALLER_SIZE; ++i) {
        auto &__ref = active_list[i + CALLEE_SIZE];
        __ref.update(__count);
    }
}


/* Tries to find a best suit for caller/callee. */
std::pair <int,int> ASMallocator::find_best_pair(int __beg,int __end) const {
    auto __next  = std::lower_bound(block_pos.begin(), block_pos.end(), __end);
    int __callee = -1, __caller = -1;

    /* If in one block, find potential hole. */
    if (__beg >= *(__next - 1)) {
        int __wasted = INT32_MAX;
        /* Find out the potential callee. */
        for(size_t i = 0 ; i < CALLEE_SIZE; ++i) {
            auto &__ref = active_list[i + 0];
            auto  __use = __ref.next_use();
            if (__use < __end) continue;
            /* Available case! Pick the least waste. */
            if (__use - __beg < __wasted) {
                __wasted = __use - __beg;
                __callee = i;
            }
        }

        __wasted = INT32_MAX;
        /* Find out the potential caller. */
        for(size_t i = 0 ; i < CALLER_SIZE; ++i) {
            auto &__ref = active_list[i + CALLEE_SIZE];
            auto  __use = __ref.next_use();
            if (__use < __end) continue;
            /* Available case! Pick the least waste. */
            if (__use - __beg < __wasted) {
                __wasted = __use - __beg;
                __caller = i;
            }
        }
    } else { /* Cannot use the holes. */
        for(size_t i = 0 ; i < CALLEE_SIZE; ++i) {
            auto &__ref = active_list[i + 0];
            auto  __use = __ref.next_use();
            if (__use == __ref.NPOS) { __callee = i; break; }
        }
        for(size_t i = 0 ; i < CALLER_SIZE; ++i) {
            auto &__ref = active_list[i + CALLEE_SIZE];
            auto  __use = __ref.next_use();
            if (__use == __ref.NPOS) { __caller = i; break; }
        }
    }

    return {__callee, __caller};
}


int ASMallocator::find_best_stack(pair_pointer __ptr) const {
    for(size_t i = 0 ; i < stack_map.size() ; ++i) {
        auto &__slot = stack_map[i];
        bool flag = false;
        for(auto [__beg,__end] : __ptr->second.live_set)
            if (!__slot.contains(__beg,__end)) { flag = true; break; }
        if (!flag) return static_cast <int> (i);
    } /* No suitable stack found. */
    return -1;
}


void ASMallocator::use_callee(pair_pointer __ptr, unsigned __callee) {
    auto &&[__vir, __info] = *__ptr;
    auto &__ref = active_list[__callee + 0];
    __ref.merge_range(__ptr);
    vir_map[__vir] = {address_info::CALLEE, __callee};
}


void ASMallocator::use_caller(pair_pointer __ptr, unsigned __caller) {
    auto &&[__vir, __info] = *__ptr;
    auto &__ref = active_list[__caller + CALLEE_SIZE];
    __ref.merge_range(__ptr);
    vir_map[__vir] = {address_info::CALLER, __caller};
}


void ASMallocator::use_stack(pair_pointer __ptr,int __idx) {
    /* Create a new place. */
    auto *__slot = stack_map.data() + __idx;
    if (__idx == -1) {
        __idx  = stack_map.size();
        __slot = &stack_map.emplace_back();
    }

    vir_map[__ptr->first] = {
        address_info::STACK, static_cast <unsigned> (__idx)
    };

    __slot->merge_range(__ptr); return;
}

/* Tries to get from a callee spill. */
std::pair <double,int> ASMallocator::get_callee_spill(int) {
    /* Never try to spill when there exists better alternative. */
    int   __spill = 0;
    double __minn = 1e20;
    /* Find the cheapest spill. */
    for(size_t i = 0 ; i < CALLEE_SIZE; ++i) {
        auto &__ref = active_list[i + 0];
        runtime_assert("This shouldn't happen!", !__ref.life_map.empty());
        /* Try to evaluate the spill. */
        auto __cost = __ref.spill_cost(false);
        if (__cost < __minn) __minn = __cost, __spill = i;
    }
    return {__minn, __spill};
}


/* Tries to get from a callee spill. */
std::pair <double,int> ASMallocator::get_caller_spill(int) {
    /* Never try to spill when there exists better alternative. */
    int   __spill = 0;
    double __minn = 1e20;
    /* Find the cheapest spill. */
    for(size_t i = 0 ; i < CALLER_SIZE; ++i) {
        auto &__ref = active_list[i + CALLEE_SIZE];
        runtime_assert("This shouldn't happen!", !__ref.life_map.empty());
        /* Try to evaluate the spill. */
        auto __cost = __ref.spill_cost(true);
        if (__cost < __minn) __minn = __cost, __spill = i;
    }
    return {__minn, __spill};
}


int ASMallocator::spill_callee(int __idx) {
    auto &__ref = active_list[__idx];
    runtime_assert("Wrong spill!", !__ref.life_map.empty());
    for(auto __ptr : __ref.life_map)
        use_stack(__ptr,find_best_stack(__ptr));

    __ref.life_map.clear();
    __ref.live_range.clear();
    return __idx;
}


int ASMallocator::spill_caller(int __idx)
{ spill_callee(__idx + CALLEE_SIZE); return __idx; }






}