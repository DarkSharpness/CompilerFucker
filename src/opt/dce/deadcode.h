#pragma once

#include "optnode.h"
#include "mem2reg.h"

namespace dark::OPT {

/**
 * @brief Eliminator for dead code in a function.
 * It will not only perform dead code elemination, but also
 * greatly simply the CFG by removing useless blocks.
 * 
 */
struct deadcode_eliminator {
    deadcode_eliminator(IR::function *__func,dominate_maker &__maker);

    struct info_collector {
        static void collect(IR::function *__func);


    };
};


}