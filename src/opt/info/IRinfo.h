#pragma once

#include "IRnode.h"

#include <bitset>
#include <unordered_set>
#include <unordered_map>

namespace dark::OPT {



/**
 * @brief The leak information of a function.
 */
struct reliance {
    /* Namespace of used constant. */
    using const_space = IR::function_argument;

    /**
     * @brief The threshold of the leak function.
     * If a function has arguments with more than this number,
     * we will not trace its data any more.
     * 
     * For any call to that function, we will always assume that
     * the information has leaked. This may lead to some missed
     * malloc -> alloca optimization, but it's not a big deal.
     * 
     * A good programmer should never write a function with so
     * many arguments. Use a class/struct wrapper instead.
     * 
    */
    static constexpr size_t THRESHOLD = 64;
    /* Only the leak bit. */
    static constexpr uint8_t LEAK_BIT = const_space::LEAK ^ const_space::USED;
    /* Only the used bit. */
    static constexpr uint8_t USED_BIT = const_space::USED;

    /* Potential function args that lead to the leak. */
    std::unordered_map <IR::function *,std::bitset <THRESHOLD>> rely_func;

    /* Just use bitset to simplify. We don't expect any extra cost. */
    static_assert(sizeof(std::bitset <THRESHOLD>) <= sizeof(size_t));

    /* Whether the information has leaked (Stored / returned out) */
    uint8_t rely_flag = const_space::DEAD;


    bool is_leak() const noexcept { return rely_flag & LEAK_BIT; }
    bool is_used() const noexcept { return rely_flag & USED_BIT; }
 
    /* Merge the using information (low bit) */
    bool merge_used (const reliance *next) {
        uint8_t temp = rely_flag;
        rely_flag   |= next->rely_flag & USED_BIT;
        return temp != rely_flag;
    }

    /* Merge the leak information. */
    bool merge_leak (const reliance *next) {
        if (is_leak() || this == next) return false;
        if (next->is_leak()) {
            rely_func.clear();
            return rely_flag = next->rely_flag;
        }

        /* None of them are leaked, so just spread the information normally. */
        rely_flag |= next->rely_flag;

        /* Now, both rely on the function.  */
        bool flag = false;
        for(auto [__func,__rhs] : next->rely_func) {
            auto &__lhs = rely_func[__func];
            auto __tmp = __lhs | __rhs;
            if (__tmp != __lhs) __lhs = __tmp, flag = true;
        } return flag;
    }


};


/**
 * @brief A mild function info collector.
 * It works on the fact that all functions are safe.
 * We will then collect the information usage of the function,
 * and spread the information brutally and expect the worst to
 * ensure the correctness of the optimization.
 * 
 * To collect more accurate data, optimize the code roughly
 * beforehand is a fucking nice idea.
 * 
 */
struct function_info {
    IR::function  *func;                /* The function information owner. */
    function_info *real_info = nullptr; /* Only one member in the SCC will hold the data. */

    explicit function_info(IR::function *__func)
    noexcept : func(__func) { __func->set_impl_ptr(this); }

    struct usage_info : hidden_impl {
        IR::node *def_node = nullptr; /* Definition node. */
    };

    /* Function related usage_info holder. */
    std::unordered_map <IR::non_literal *,usage_info> use_map;

    /* The functions that this function directly calls. */
    std::unordered_set <IR::function *> called_func;
    /* The functions that directly call this function.  */
    std::unordered_set <IR::function *> caller_func;


    size_t dfn = NPOS; /* Used in tarjan. */
    size_t low = NPOS; /* Used in tarjan. */
    size_t scc = NPOS; /* Used in tarjan. */


    /**
     * -----------------------------------------------------------
     * The information above is linked to the function only. 
     * The information below is shared by all members in the SCC.
     * -----------------------------------------------------------
    */

    /* All functions that this function calls. */
    std::unordered_set <IR::function *> recursive_func;
    /* All global variables that this function will use. */
    std::unordered_map <IR::global_variable *,uint8_t> used_global_var;

    /**
     * Whether the function may call itself.
     * This is a special case in SCC:  a node with its scc_size = 1,
     * which means that it's a single node SCC. In this case, we have
     * to record whether the function calls itself.
    */
    bool self_call() const noexcept { return recursive_func.count(func); } 

    /* All usage data of the used global_variable. */
    void merge_within_SCC(const function_info &);
    void merge_between_SCC(const function_info &);


    /**
     * -----------------------------------------------------------
     * The information below is some static constant definition.
     * -----------------------------------------------------------
    */


    static constexpr uint8_t NONE     = 0b00; /* Not used in any expression.    */
    static constexpr uint8_t LOAD     = 0b01; /* Load the value.                */
    static constexpr uint8_t STORE    = 0b10; /* Store the value.               */
    static constexpr uint8_t BOTH     = 0b11; /* Both load and store.           */
    static constexpr size_t  NPOS     = -1;   static_assert(!(NPOS + 1));
};


}