#include "IRinfo.h"



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