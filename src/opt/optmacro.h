#pragma once

#include <string>
#include <stdexcept>

namespace dark::OPT {


class optimize_options {
    optimize_options() = delete;

    /* Optimization level */
    inline static char OPT_LEVEL   = 0;

    /* Dead code elimination. */
    inline static char ENABLE_DCE  = -1;

    /* Constant propagation. */
    inline static char ENABLE_SCCP = -1;

 public:

    static void init(int argc,const char** argv) {
        /* Only called once. */
        for(int i = 1 ; i < argc ; ++i) {
            std::string_view __view = argv[i];
            /* Invalid command. */
            if(__view.size() <= 2 || __view.substr(0, 2) != "--")
                throw std::runtime_error("Invalid option: " + std::string(__view));
            __view.remove_prefix(2);

            /* Optimize level (only 0 ~ 3 available) */
            if(__view[0] == 'O') OPT_LEVEL = __view[1] - '0';
            else try_parse_command(__view);
        }

        switch(OPT_LEVEL) {
            case 0: update_DCE(0); update_SCCP(0); break;
            case 1: update_DCE(1); update_SCCP(0); break;
            case 2: update_DCE(1); update_SCCP(1); break;
            case 3: update_DCE(1); update_SCCP(1); break;
            default: throw std::runtime_error("Invalid optimization level.");
        }
    }

    static bool DCE_enabled()  { return ENABLE_DCE  == 1; }
    static bool SCCP_enabled() { return ENABLE_SCCP == 1; }

  private:

    static void throw_duplicated(std::string __str) {
        throw std::runtime_error("Duplicated option: " + __str);
    }

    static void update_DCE(char __val) {
        if(ENABLE_DCE == -1) ENABLE_DCE = __val;
        else if(ENABLE_DCE != __val) throw_duplicated("DCE");
    }

    static void update_SCCP(char __val) {
        if(ENABLE_SCCP == -1) ENABLE_SCCP = __val;
        else if(ENABLE_SCCP != __val) throw_duplicated("SCCP");
    }

    static void try_parse_command(std::string_view __view) {
        auto __pos = __view.find('-');
        auto __cmd = __view.substr(0,__pos);
        auto __ctx = __view.substr(__pos + 1);
        if(__cmd == "disable") {
            if(__ctx == "dce")       return update_DCE(0);
            else if(__ctx == "sccp") return update_SCCP(0);
        } else if(__cmd == "enable") {
            if(__ctx == "dce")       return update_DCE(1);
            else if(__ctx == "sccp") return update_SCCP(1);
        }

        throw std::runtime_error("Invalid option: " + std::string(__view));
    }
};



}