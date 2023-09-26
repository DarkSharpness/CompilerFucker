#pragma once

#include "IRnode.h"

#include <array>
#include <unordered_map>

namespace dark::IR {


/**
 * This is a special helper class!
 * 
 * __array__.size(this)                 = 0
 * string.length(this)                  = 1
 * string.substring(this,int l,int r)   = 2
 * string.parseInt(this)                = 3
 * string.ord(this,int n)               = 4
 * .print(string str)                   = 5
 * .println(string str)                 = 6
 * .printInt(int n)                     = 7
 * .printlnInt(int n)                   = 8
 * .getString()                         = 9
 * .getInt()                            = 10
 * .toString(int n)                     = 11
 * .__string__add__(string a,string b)  = 12
 * 
 * .__new_array1__ (int length)         = 19
 * .__new_array4__ (int length)         = 20
 * malloc(int size)                     = 21
 * strcmp(string lhs,string rhs)        = 22
*/
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

    /**
     * @brief Return the builtin index of a builtin function.
    */
    static size_t get_builtin_index(IR::function *__func) {
        size_t __n = __func - builtin_function.data();
        if (__n >= BUITLIN_SIZE) throw error("Wrong builtin function!");
        return __n;
    }


};




}