#pragma once

#include "ASMnode.h"

namespace dark::ASM {

struct ASMliveanalyser {
    struct usage_info {
        int use_beg = INT32_MAX;    /* Beginning of usage.  */
        int use_end = INT32_MIN;    /* Ending of usage.     */
        int blk_beg = INT32_MAX;    /* Beginning of block. */
        int blk_end = INT32_MIN;    /* Ending of block. */

        /**
         * Weight of usage. It is computed as below:
         * A spilled use is counted as 1.
         * 
         * Weight inside one loop is multipled by 10.
        */
        size_t use_weight = 0;

        /**
         * Weight of call. (Due to calling convention).
         * 
         * A call between in live interval depends on whether
         * the function may affect temporary registers, 
         * thus one call spill is counted as 2.
         * 
         * Weight inside one loop is multipled by 10.
        */
        size_t call_weight = 0;

        /* All function call that may bring a cost to it. */
        std::vector <function_node *> save_set;

        void init_save_set();
        /* Whether this argument live through the function. */
        bool need_saving(function_node *) const noexcept;

        /* Update the init block count and beg/end order. */
        void update(int __blk,int __ord) {
            if (__ord < use_beg) use_beg = __ord;
            if (__ord > use_end) use_end = __ord;
            if (__blk < blk_beg) blk_beg = __blk;
            if (__blk > blk_end) blk_end = __blk;
        }

        double distance_factor(int) const noexcept;
        double get_spill_weight(int) const noexcept;
        double get_caller_weight(int) const noexcept;
        double get_callee_weight(int) const noexcept;
    };

    std::unordered_map <virtual_register *, usage_info> usage_map;

    /* Analyze one function. */
    ASMliveanalyser(function *);

    static void init_count(std::vector <block *> &);
    static void make_order(function *);

};

}