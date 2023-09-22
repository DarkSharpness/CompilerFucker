#pragma once

#include "ASMnode.h"
#include "ASManalyser.h"

namespace dark::ASM {


/* Linear scan allocator. */
struct ASMallocator {
    ASMliveanalyser *__data;
    ASMallocator(function *);
};


}