#pragma once
#include "optnode.h"



namespace dark::OPT {


struct loop_info {
    IR::block_stmt *header = {};
    std::vector <IR::temporary *> induction_variables;


};


}