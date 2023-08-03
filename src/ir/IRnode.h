#pragma once

#include "scope.h"

#include <string>
#include <vector>


namespace dark::IR {

/* A special method. */
using c_string = char [8];

/* enum class will fail here! */
enum typeinfo : unsigned char {
    PTR  = 0, /* string/array/reference  */
    I32  = 1, /* int  */
    I1   = 2, /* bool */
    VOID = 3, /* void */
};
/* Type to name mapping. */
inline constexpr c_string type_map[4] = {
    [typeinfo::PTR]  = {'p','t','r'},
    [typeinfo::I32]  = {'i','3','2'},
    [typeinfo::I1]   = {'i','1'},
    [typeinfo::VOID] = {'v','o','i','d'}
};


/* Return the standard name of local temporary register. */
inline std::string get_local(size_t &__n) { return '%' + std::to_string(++__n); }


struct node {
    virtual std::string data() = 0;
    virtual ~node() = default;
};

/* A definition can be variable / literal */
struct definition : node {
    virtual typeinfo get_type() = 0;
};

/* Variables or temporaries. */
struct variable : definition {
    std::string name;
    typeinfo    type;
    typeinfo get_type() override final { return type; }
    std::string  data() override final { return name; }
    ~variable()         override = default;
};

struct local_var  : variable {
    ~local_var() override = default;
};

struct global_var : variable {
    ~global_var() override = default;
};

struct temporary  : variable {
    ~temporary() override = default;
};

/* Literal constants. */
struct literal  : definition {};

struct string_constant : literal {
    size_t index; /* The index of the string_constant. */
    typeinfo get_type() override { return typeinfo::PTR; }
    std::string  data() override { return "@str-" + std::to_string(index); }
    ~string_constant()  override = default;
};

struct integer_constant : literal {
    int value;
    typeinfo get_type() override { return typeinfo::I32; }
    std::string  data() override { return std::to_string(value); }
    ~integer_constant() override = default;
};

struct boolean_constant : literal {
    bool value;
    typeinfo get_type() override { return typeinfo::I1; }
    std::string  data() override { return value ? "true" : "false"; }
    ~boolean_constant() override = default;
};

struct null_constant : literal {
    typeinfo get_type() override { return typeinfo::PTR; }
    std::string  data() override { return "null"; }
    ~null_constant()    override = default;
};


struct statement  : node {};

struct binary_stmt : statement {
    enum : unsigned char {
        ADD  = 0,
        SUB  = 1,
        MUL  = 2,
        SDIV = 3,
        SREM = 4,
        SHL  = 5,
        ASHR = 6,
        AND  = 7,
        OR   = 8,
        XOR  = 9,
    } op;
    inline static constexpr c_string str[10] = {
        [ADD]   = {'a','d','d'},
        [SUB]   = {'s','u','b'},
        [MUL]   = {'m','u','l'},
        [SDIV]  = {'s','d','i','v'},
        [SREM]  = {'s','r','e','m'},
        [SHL]   = {'s','h','l'},
        [ASHR]  = {'a','s','h','r'},
        [AND]   = {'a','n','d'},
        [OR]    = {'o','r'},
        [XOR]   = {'x','o','r'},
    };
    typeinfo type;

    size_t  temp_var;
    definition *lvar;
    definition *rvar;

    /* <result> = <operator> <type> <operand1>, <operand2> */
    std::string data() override {
        return string_join(temp_var," = ",str[op]," i32 ",lvar->data(),", ",rvar->data(),'\n');
    };

    ~binary_stmt() override = default;
};


struct unary_stmt : statement {
    enum : unsigned char {
        ADD = 0, /* This will do nothing.     */
        SUB = 1, /* This will simply go nega. */
        REV = 2, /* This will simply reverse. */
    } op;

    size_t  temp_var;
    definition *rvar;

    std::string data() override {
        if(op == ADD) return "";
        else if(op == SUB) {
            return string_join(temp_var," = sub i32  0, ",rvar->data());
        } else {
            return string_join(temp_var," = xor i32 -1, ",rvar->data());
        }
    };

    ~unary_stmt() override = default;
};



struct branch_stmt : statement {

};


struct loop_stmt : node {

};


struct call_stmt : node {

};


struct function : node {
    typeinfo    type;
    std::string name;

    size_t temp_count = 0;
    size_t bran_count = 0;
    size_t loop_count = 0;

    std::vector <variable  *> args;
    std::vector <statement *> args;

    /* define <ResultType> @<FunctionName> (...) {...} */
    std::string data() override {
        std::string __tmp; /* Arglist. */
        for(auto __p : args) {
            if(__p != args.front())
                __tmp += ',';
            __tmp += type_map[__p->type];
            __tmp += ' ';
            __tmp += __p->data();
        }
        std::string __body;

        return string_join("define ",type_map[type]," @",name,
                           " (",std::move(__tmp),") {\nentry:",
                           std::move(__body),"}\n\n");
    };


    ~function() override = default;
};




}