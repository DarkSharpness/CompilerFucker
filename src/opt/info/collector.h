#pragma once

#include "optnode.h"
#include "IRinfo.h"

#include <deque>

namespace dark::OPT {


struct info_collector {
    template <bool __time>
    info_collector(function_info &,std::integral_constant<bool,__time>);
};


struct function_graph {
    function_graph() = delete;
    inline static size_t scc_count = 0;
    inline static size_t dfn_count = 0;

    static void tarjan(function_info &);
    static bool work_topo(std::deque <function_info> &);
    static void resolve_dependency(std::deque <function_info> &);

};



}

