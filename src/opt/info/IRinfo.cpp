#include "IRinfo.h"

namespace dark {

/**
 * @brief Whether a function call is related to global variable
 * or side effective to the data.
 * @return 0 If no side effect 
 *      || 1 ~ 3 If builtin IO-function.
        || 4 If all potential side effects. (No func visited yet).
        || 5 If partial side effects. (All func_info are collected.)
 */
size_t IR::function::is_side_effective() const {
    /* Only those linked to inout are side effective. */ 
    if (is_builtin) return inout_state;

    /* No function info has been collected: error! */
    auto __ptr = get_impl_ptr <OPT::function_info> ();
    if (!__ptr || !__ptr->real_info) return 4;

    warning("Not implemented yet.");
    return 5;
}

}

namespace dark::OPT {

/**
 * @brief Merge the global data of 2 function.
 * @param __lhs Left hand side function.
 * @param __rhs Right hand side function.(Will no be touched)
 */
void merge_inner_data(function_info &__lhs, const function_info &__rhs) {
    __lhs.func->inout_state |= __rhs.func->inout_state;
    for(auto &&__global : __rhs.used_global_var) {
        auto [__iter,__result] = __lhs.used_global_var.emplace(__global);
        if (!__result) __iter->second |= __global.second;
    }
}


/**
 * @brief Merge the information in one SCC.
 * 
*/
void function_info::merge_within_SCC(const function_info &__next) {
    recursive_func.insert(__next.func);
    if (this == &__next) return;
    merge_inner_data(*this,__next);
}

/**
 * @brief Merge the information of two function together.
 * This operation works on the recursive set.
 * 
 */
void function_info::merge_between_SCC(const function_info &__next) {
    runtime_assert("wtf?",this != &__next);
    for(auto __func : __next.recursive_func)
        recursive_func.insert(__func);
    merge_inner_data(*this,__next);
}


}