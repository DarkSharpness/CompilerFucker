#pragma once

#include "IRnode.h"

#include <array>
#include <unordered_map>

namespace dark::IR {


struct IRbasic {
    /* Count of all builtin functions (including some wasted functions awa.) */
    inline static constexpr size_t BUITLIN_SIZE = 23;

    using _Function_List = std::array         <function,BUITLIN_SIZE>;
    using _String_Map    = std::unordered_map <std::string,global_variable *>;

    /* This can be touched even outside the scope! */
    inline static _Function_List  builtin_function  = {};
    inline static _String_Map        string_map     = {};

    /**
     * Create a c-style string constant.
     * Note that literal pools should be managed globally.
     */
    static IR::global_variable *create_cstring(const std::string &__name) {
        auto [__iter,__result] = string_map.insert({__name,nullptr});
        if(!__result) return __iter->second;

        auto *__str = new string_constant {__name};

        /* This is a special variable! */
        auto *__var = new global_variable;
        __var->name = "@str." + std::to_string(string_map.size());
        __var->type = { & __string_class__ , 1 };

        /* Bi-directional linking. */
        __var->const_val  = __str;
        __iter->second    = __var;

        /* In the form of var, __str. */
        return __var;
    }
};




}