#pragma once

#include <iostream>
#include <string>
#include <stdexcept>
#include <cstdint>

namespace dark::OPT {


class optimize_options {
  public:
    struct optimize_info {
        union {
            struct {
                uint64_t optimize_level : 2;
                uint64_t enable_DCE     : 1;
                uint64_t enable_SCCP    : 1;
                uint64_t enable_CFG     : 1;
                uint64_t enable_PEEP    : 1;
                uint64_t enable_INLINE  : 1;
                /* Useless padding. */
            };
            size_t __init__; /* This is used to help init. */
        };
        /* Initialize with optimize level. */
        explicit optimize_info(size_t __n = 0) { optimize_level = __n; }
        /**
         * @brief Initiailize the data within.
         * 
         * @param __DCE  Dead code elimination.
         * @param __SCCP Sparse conditional constant propagation.
         * @param __CFG  Control flow graph simplification.
         */
        constexpr explicit optimize_info(
            bool __DCE,
            bool __SCCP,
            bool __CFG,
            bool __PEEP,
            bool __INLINE
        ) noexcept : __init__(0) {
            enable_DCE  = __DCE;
            enable_SCCP = __SCCP;
            enable_CFG  = __CFG;
            enable_PEEP = __PEEP;
            enable_INLINE = __INLINE;
        }

        /* Return whether any optimization is enabled. */
        bool is_enabled() const noexcept { return __init__ != 0; }
    };
  private:
    optimize_options() = delete;

    
    /* Default optimization level. */
    inline static constexpr size_t LVL = 3; 
    /* Default as unmodified. */
    inline static optimize_info _init  = optimize_info { 0 };
    /* Default with -O3 enabled. */
    inline static optimize_info state  = optimize_info {LVL};

  public:

    static void init(int argc,const char** argv) {
        if (argc <= 1) return update_optimize_level(LVL);
        static bool __call_once = true;
        if(!__call_once)
            throw std::runtime_error("optimize_options::init() called twice.");
        __call_once = false;

        size_t __level = state.optimize_level;
        /* Only called once. */
        for(int i = 1 ; i < argc ; ++i) {
            std::string_view __view = argv[i];
            /* Invalid command. */
            if(__view.size() <= 2 || __view.substr(0, 2) != "--")
                throw_invalid_option(__view);
            __view.remove_prefix(2);

            /* Optimize level (only 0 ~ 3 available) */
            if(__view[0] == 'O') {
                if(__view.size() != 2) throw_invalid_option(__view);
                __level = __view[1] - '0';
            } else try_parse_command(__view);
        }

        update_optimize_level(__level);
    }

    static auto get_state() noexcept { return state; }

  private:

    static void update_optimize_level(size_t __n) {
        if(__n > 3) throw std::runtime_error("Invalid optimization level.");
        static const optimize_info update_map[] = {
            optimize_info { 0 , 0 , 0 , 0 , 0},
            optimize_info { 1 , 1 , 1 , 0 , 0},
            optimize_info { 1 , 1 , 1 , 1 , 1},
            optimize_info { 1 , 1 , 1 , 1 , 1},
        };
        size_t __mask__ = _init.__init__;
        state. __init__ = /* Special method. */
            (state.__init__ & __mask__) | (update_map[__n].__init__ & ~__mask__);

        state.optimize_level = __n;
    }

    static void throw_invalid_option(std::string_view __view) {
        throw std::runtime_error("Invalid option: " + std::string(__view));
    }

    static void throw_duplicate_option(std::string __str) {
        throw std::runtime_error("Duplicate option: " + __str);
    }

    static void update_DCE(bool __val) {
        if(_init.enable_DCE) throw_duplicate_option("DCE");
        state.enable_DCE = __val;
        _init.enable_DCE = true;
    }

    static void update_SCCP(bool __val) {
        if(_init.enable_SCCP) throw_duplicate_option("SCCP");
        state.enable_SCCP = __val;
        _init.enable_SCCP = true;
    }

    static void update_CFG(bool __val) {
        if(_init.enable_CFG) throw_duplicate_option("CFG");
        state.enable_CFG = __val;
        _init.enable_CFG = true;
    }

    static void update_PEEP(bool __val) {
        if(_init.enable_PEEP) throw_duplicate_option("PEEP");
        state.enable_PEEP = __val;
        _init.enable_PEEP = true;
    }

    static void update_INLINE(bool __val) {
        if(_init.enable_INLINE) throw_duplicate_option("INLINE");
        state.enable_INLINE = __val;
        _init.enable_INLINE = true;
    }

    static void try_parse_command(std::string_view __view) {
        auto __pos = __view.find('-');
        auto __cmd = __view.substr(0,__pos);
        auto __ctx = __view.substr(__pos + 1);
        if(__cmd == "disable") {
            if(__ctx == "DCE")       return update_DCE(0);
            else if(__ctx == "SCCP") return update_SCCP(0);
            else if(__ctx == "CFG")  return update_CFG(0);
            else if(__ctx == "PEEP") return update_PEEP(0);
            else if(__ctx == "INLINE") return update_INLINE(0);
        } else if(__cmd == "enable") {
            if(__ctx == "DCE")       return update_DCE(1);
            else if(__ctx == "SCCP") return update_SCCP(1);
            else if(__ctx == "CFG")  return update_CFG(1);
            else if(__ctx == "PEEP") return update_PEEP(1);
            else if(__ctx == "INLINE") return update_INLINE(1);
        }
        throw_invalid_option(__view);
    }
};



}