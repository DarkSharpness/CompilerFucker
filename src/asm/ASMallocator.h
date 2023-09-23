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
            STACK       = 0,    /* Spilled into stack.      */
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

    inline static constexpr size_t CALLEE_SIZE = 14;
    inline static constexpr size_t CALLER_SIZE = 14;

    /**
     * Remapping :
     * callee 0 ~ 13    Saved
     * caller 14 ~ 27   Temporaries
    */
    struct register_slot {
        static inline constexpr int NPOS = INT32_MAX - 2;
        /* A map to the death time of a virtual register within. */
        std::vector <pair_pointer> life_map;
        /* The sum of all live ranges. */
        std::list <std::pair <int,int>> live_range;
        /* Return the next use index. */
        int next_use() const noexcept;
        double spill_cost(bool) const noexcept;
        void merge_range(pair_pointer);
        bool update(const int) noexcept;
    } active_list[CALLEE_SIZE + CALLER_SIZE];

    struct stack_slot {
        /* The sum of all live ranges. */
        std::set <std::pair <int,int>> live_range =
            { {-1,-1}, {INT32_MAX,INT32_MAX} };
        bool contains(int,int) const noexcept;
        void merge_range(pair_pointer);
    };

    std::vector <stack_slot> stack_map;


    void expire_old(int);
    void linear_allocate(pair_pointer);

    std::pair <int,int> find_best_pair(int,int) const;
    int find_best_stack(pair_pointer) const;

    std::pair <double,int> get_callee_spill(int);
    std::pair <double,int> get_caller_spill(int);

    int spill_callee(int);
    int spill_caller(int);

    void use_callee(pair_pointer,unsigned);
    void use_caller(pair_pointer,unsigned);
    void use_stack(pair_pointer,int);

};


}