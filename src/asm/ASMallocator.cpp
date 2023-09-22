#include "ASMallocator.h"


namespace dark::ASM {

double ASMliveanalyser::usage_info::distance_factor
    (int __blk) const noexcept {
    return static_cast <double> (blk_end - __blk) * 8 + (use_end - use_beg);
}

/* Callee save: No cost. */
double ASMliveanalyser::usage_info::get_callee_weight
    (int __blk) const noexcept { return 0; }
/* Caller save: Temporaries. */
double ASMliveanalyser::usage_info::get_caller_weight
    (int __blk) const noexcept  {
    return static_cast <double> (call_weight)
            / distance_factor(__blk);
}
/* Spilled: Use/Def count as weight.  */
double ASMliveanalyser::usage_info::get_spill_weight
    (int __blk) const noexcept {
    return static_cast <double> (use_weight)
            / distance_factor(__blk);
}

ASMallocator::ASMallocator(function *__func) {
    __data = new ASMliveanalyser {__func};


}


}