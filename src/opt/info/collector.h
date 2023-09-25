#pragma once

#include "optnode.h"
#include "IRinfo.h"

#include <deque>

namespace dark::OPT {


/**
 * @brief This container must satisfy the condition that
 * all its poiners won't be invalidated after push_back.
 * 
 * Naturally, std::deque or std::list is a good choice.
 * 
 */
using _Info_Container = std::deque <function_info>;


struct info_collector {
    info_collector(function_info &);
    static void build(_Info_Container &);
};



struct function_graph {
    /* Since there won't be too many functions, this is not a bad hash. */
    struct my_hash {
        size_t operator () (std::pair <size_t,size_t> __pair)
        const noexcept {
            return __pair.first << 32 | __pair.second;
        }
    };

    using _Edge_Set = std::unordered_set <std::pair <size_t,size_t>,my_hash>;
    using _Edge_In  = std::vector <std::vector <size_t>>;
    using _Degrees  = std::vector <size_t>;
    using _Init_Dat = struct {
        function_info *main;
        function_info *init;
    };

    inline static _Edge_In  edge_in     = {};
    inline static _Degrees  degree_out  = {};
    inline static _Edge_Set edge_set    = {};
    inline static _Init_Dat __init_pair = {{},{}};

    function_graph() = delete;
    inline static size_t scc_count = 0;
    inline static size_t dfn_count = 0;

    static void tarjan(function_info &);

    static void build_graph(const _Info_Container &);

    static bool work_topo(_Info_Container &);

    static void resolve_leak(_Info_Container &);

    static void resolve_used(_Info_Container &);

    static std::vector <IR::function *> inline_order(_Info_Container &);
};




}

