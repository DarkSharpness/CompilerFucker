#pragma once
#include "optnode.h"
#include "loopbase.h"

#include <unordered_set>
#include <unordered_map>

namespace dark::OPT {


struct loop_detector {
    struct usage_info {
        IR::definition *def_node = nullptr;
        size_t         use_count = 0;
    };

    std::unordered_map <node *,loop_info>  loop_map;
    loop_detector(IR::function *,node *);

};


}