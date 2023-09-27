#pragma once
#include "optnode.h"

#include <unordered_set>


namespace dark::OPT {


struct loop_info {
    node *header = {};
    std::vector <IR::temporary *> induction_variables;
    std::unordered_set <node *>   loop_blocks;
};


}