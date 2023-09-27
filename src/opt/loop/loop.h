#pragma once
#include "optnode.h"
#include "loopbase.h"

#include <unordered_set>
#include <unordered_map>

namespace dark::OPT {


struct loop_detector {
    std::unordered_map <node *,loop_info>  loop_map;
    loop_detector(IR::function *,node *);

};


}