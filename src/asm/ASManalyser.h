#pragma once

#include "ASMnode.h"

namespace dark::ASM {

struct ASMliveanalyser  {
    /* Analyze one function. */
    ASMliveanalyser(function *);

    static void make_order(function *);

};

}