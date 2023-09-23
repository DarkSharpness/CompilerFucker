#pragma once

#include "ASMnode.h"

namespace dark::ASM {

struct ASMlifeanalyser {
    struct usage_info {
        /**
         * Weight of usage. It is computed as below:
         * A spilled use is counted as 1.
         * 
         * Weight inside one loop is multipled by 10.
        */
        double use_weight = 0;

        /**
         * Weight of call. (Due to calling convention).
         * 
         * A call between in live interval depends on whether
         * the function may affect temporary registers, 
         * thus one call spill is counted as 2.
         * 
         * Weight inside one loop is multipled by 10.
        */
        double call_weight = 0;

        /* Length of live interval. */
        size_t life_length = 0;

        using live_interval = std::pair <int, int>;

        /* All function call that may bring a cost to it. */
        std::vector <function_node *> save_set;
        /* All intervals that the virtual is dead. */
        std::vector < live_interval > live_set;

        void init_set();
        bool need_saving(function_node *) const noexcept;
        int  get_state(int) const noexcept;
        int  get_begin()    const noexcept;
        int  get_end()      const noexcept;

        double distance_factor(int) const noexcept;
        double get_spill_weight(int) const noexcept;
        double get_caller_weight(int) const noexcept;
        double get_callee_weight(int) const noexcept;
    };

    std::unordered_map <virtual_register *, usage_info> usage_map;

    /* Analyze one function. */
    ASMlifeanalyser(function *);

    static void init_count(std::vector <block *> &);
    static void make_order(function *);

    void debug_print(std::ostream &,function *);
};

}