#pragma once

#include "optnode.h"

#include <unordered_map>

namespace dark::OPT {


struct special_judger : hidden_impl {
    struct usage_info {
        size_t use_count            = {};
        IR::definition *def_node    = {};
    };

    std::unordered_map <IR::temporary *, usage_info> usage_map;
    special_judger(IR::function *, node *,void *);
};



}