#pragma once

#include "ASMnode.h"
#include "ASManalyser.h"

#include <list>

namespace dark::ASM {


/* Linear scan allocator. */
struct ASMallocator {
    const ASMlifeanalyser *__impl;
    ASMallocator(function *);

    struct address_info {
        enum : unsigned {
            IN_STACK    = 0,    /* Spilled into stack.      */
            CALLER      = 1,    /* Caller save register     */
            CALLEE      = 2,    /* Callee save register     */
            TEMPORARY   = 3     /* Temporary register saved */
        } type;
        unsigned index; /* Index in stack/register/temporary */
    };

    using pair_pointer = decltype(__impl->usage_map.begin());

    /* Index of block position. */
    std::vector <int> block_pos;

    /* Worklist sorted by ascending beginning order */
    std::vector <pair_pointer> work_list;

    /* Map a virtual register to its index. */
    std::unordered_map <virtual_register *,address_info> vir_map;

    template <size_t __N>
    struct register_pack {
        /* Available list. */
        unsigned avail[__N];
        unsigned top; /* Current value. */
        /* Pack of registers. */
        explicit register_pack() noexcept : top(__N) {
            for (unsigned i = 0; i < __N; ++i) avail[i] = i;
        }

        unsigned *begin() noexcept { return avail; }
        unsigned *end() noexcept { return avail + top; }

        /* Return count of available register. */
        unsigned size() const noexcept { return __N; }
        unsigned allocate() { return avail[--top]; }
        void deallocate(unsigned __i) { avail[top++] = __i; }
    };

    inline static constexpr size_t CALLEE_SIZE = 14;
    inline static constexpr size_t CALLER_SIZE = 14;

    /**
     * Remapping :
     * callee 0 ~ 13
     * caller 14 ~ 27
    */
    struct {
        using usage_info = ASMlifeanalyser::usage_info;
        /* A map to the death time of a virtual register within. */
        std::vector <const usage_info *> life_map;

        /* The sum of all live ranges. */
        std::list <std::pair <int,int>> live_range;

        /* Return the next use index. */
        int next_use() const noexcept {
            return live_range.empty() ? -1 : live_range.front().first;
        }

        /* Merge range to the front. */
        void merge_range(const usage_info *__use) {
            life_map.push_back(__use);
            auto &&__range = __use->live_set;
            std::list <std::pair <int,int>> __new_range
                = { __range.begin(), __range.end() };
            __new_range.splice(__new_range.end(), live_range);
            live_range = std::move(__new_range);
        }

        /* Update to test whether it is newly expired. */
        bool update(const int __n) noexcept {
            if (live_range.empty()) return false;

            /* Update life map first. */
            auto __beg = life_map.begin();
            auto __end = life_map.end();
            const auto __bak = __beg;    
            while(__beg != __end) {
                if ((*__beg)->get_end() <= __n)
                    *__beg = *--__end;
                else ++__beg;
            } life_map.resize(__end - __bak);
            /* Nothing is left alive...... */
            if (__end == __bak) { live_range.clear(); return true; }

            do {
                auto &__front = live_range.front();
                /* If no exceeding the end, do nothing. */
                if (__front.second > __n) {
                    __front.first = std::max(__front.first,__n);
                    return false;
                } else live_range.pop_front();
            } while(true);
        }
    } active_list[CALLEE_SIZE + CALLER_SIZE];

    /**
     * Callee saved registers.
     * s0 ~ s11 and gp, tp
     * 14 in total.
     */
    register_pack <CALLEE_SIZE> callee_pool {};

    /**
     * Caller saved registers.
     * t2 ~ t6, a0 ~ a7, ra
     * 14 in total. (t0 ~ t1 is reserved for temporary use)
    */
    register_pack <CALLER_SIZE> caller_pool {};


    void expire_old(int);
    void linear_allocate(pair_pointer);

    std::pair <int,int> find_best_suit(int,int);
    unsigned get_callee_spill(int) { return 0; }
    unsigned get_caller_spill(int) { return 0; }
    void use_callee(pair_pointer,unsigned);
    void use_caller(pair_pointer,unsigned);

};


}