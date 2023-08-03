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


struct class_type {
    std::string name; /* The name contains '%' */
    std::vector <typeinfo *> layout;
};

struct label_type { std::string label; };

struct statement  : node {};

/* Block statement is not a statement! */
struct block_stmt : label_type  {
    std::vector <statement *> stmt; /* Statements */

    /* Simply join all message in it together. */
    std::string data() {
        std::vector <std::string> __tmp;

        __tmp.reserve(stmt.size() + 2);
        __tmp.push_back(label + ":\n");
        for(auto __p : stmt) __tmp.push_back(__p->data());
        __tmp.push_back("\n");

        return string_join_array(__tmp.begin(),__tmp.end());
    }
};


/* A definition can be variable / literal */
struct definition : node {
    virtual typeinfo get_type() = 0;
};

/* Variables or temporaries. */
struct value_type : definition {
    std::string name;
    typeinfo    type;
    typeinfo get_type() override final { return type; }
    std::string  data() override final { return name; }
};


/* Pure variables. */
struct variable   : value_type {};

struct local_var  : variable {
    ~local_var() override = default;
};

struct global_var : variable {
    ~global_var() override = default;
};

struct temporary  : value_type {
    ~temporary() override = default;
};

/* Literal constants. */
struct literal    : definition {};

struct null_constant : literal {
    typeinfo get_type() override { return typeinfo::PTR; }
    std::string  data() override { return "null"; }
    ~null_constant()    override = default;
};

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


struct function : node {
    std::string name; /* Function name. */

    size_t temp_count = 0; /* This is used to help generate IR. */
    size_t bran_count = 0; /* This is used to help generate IR. */
    size_t loop_count = 0; /* This is used to help generate IR. */

    std::vector <variable *>   args; /* Argument list. */
    std::vector <block_stmt *> stmt; /* Body data.   */
    typeinfo                   type; /* Return type. */

    /* define <ResultType> @<FunctionName> (...) {...} */
    std::string data() override {
        std::string __arg; /* Arglist. */
        for(auto __p : args) {
            if(__p != args.front())
                __arg += ',';
            __arg += type_map[__p->get_type()];
            __arg += ' ';
            __arg += __p->data();
        }

        std::vector <std::string> __tmp;

        __tmp.reserve(stmt.size() + 2);
        __tmp.push_back(string_join(
            "define ",type_map[type]," @",name,
            " (",std::move(__arg),") {\n"
        ));
        for(auto __p : stmt) __tmp.push_back(__p->data());
        __tmp.push_back(string_join("}\n\n"));

        return string_join_array(__tmp.begin(),__tmp.end());
    };

    ~function() override = default;
};


struct compare_stmt : statement {
    enum : unsigned char {
        EQ = 0,
        NE = 1,
        GT = 2,
        GE = 3,
        LT = 4,
        LE = 5,
    } op;
    inline static constexpr c_string str[6] = {
        [EQ] = {'e','q'},
        [NE] = {'n','e'},
        [GT] = {'s','g','t'},
        [GE] = {'s','g','e'},
        [LT] = {'s','l','t'},
        [LE] = {'s','l','e'}
    };

    temporary  *dest;
    definition *lvar;
    definition *rvar;

    /* <result> = icmp <cond> <type> <operand1>, <operand2> */
    std::string data() override {
        return string_join(dest->data()," = icmp "
        ,str[op],' ',type_map[lvar->get_type()],' '
        ,lvar->data(),", ",rvar->data(),'\n');
    };
};


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

    temporary  *dest;
    definition *lvar;
    definition *rvar;

    /* <result> = <operator> <type> <operand1>, <operand2> */
    std::string data() override {
        return string_join(dest->data()," = "
        ,str[op]," i32 ",lvar->data(),", ",rvar->data(),'\n');
    };

    ~binary_stmt() override = default;
};


/* Unconditional jump. */
struct jump_stmt : statement {
    block_stmt *dest;

    /* br label <dest> */
    std::string data() override {
        return string_join("br label %",dest->label,'\n');
    }
};


/* Conditional branch. */
struct branch_stmt : statement {
    definition *cond;  /* The condition variable. */
    block_stmt *br[2]; /* Label with a name. */

    /* br i1 <cond>, label <iftrue>, label <if_false> */
    std::string data() override {
        return string_join(
            "br i1",cond->data(),
            ", label %",br[1]->label,
            ", label %",br[0]->label,
            '\n');
    }

    ~branch_stmt() override = default;
};

struct call_stmt : statement {
    function  *func;
    temporary *dest;
    std::vector <definition *> args;

    /* <result> = call <ResultType> @<FunctionName> (<argument>) */
    std::string data() override {
        std::string __arg; /* Arglist. */
        for(auto __p : args) {
            if(__p != args.front())
                __arg += ',';
            __arg += type_map[__p->get_type()];
            __arg += ' ';
            __arg += __p->data();
        }
        std::string __prefix;
        if(func->type != typeinfo::VOID)
            __prefix = dest->data() + " = ";
        return string_join(
            __prefix, "call ",
            type_map[func->type]," @",func->name,
            " (",std::move(__arg),")\n"
        );
    }
    ~call_stmt() override = default;
};

struct load_statement : statement {
    value_type *src;
    temporary  *dst;

    /* <result> = load <type>, ptr <pointer>  */
    std::string data() override {
        return string_join(
            dst->data(), " = load ",
            type_map[dst->get_type()],", ptr ",src->data(),'\n'
        );
    }
};


struct store_stmt : statement {
    definition *src;
    variable   *dst;

    /* store <type> <value>, ptr <pointer>  */
    std::string data() override {
        return string_join(
            "store ",type_map[dst->get_type()],' ',
            src->data(),", ptr ",dst->data(),'\n'
        );
    }

    ~store_stmt() override = default;
};


struct return_stmt : statement {
    definition *rval;

    std::string data() override {
        if(!rval) return "ret void\n";
        else {
            return string_join(
                "ret ",type_map[rval->get_type()],' ',rval->data(),'\n'
            );
        }
    }
    ~return_stmt() override = default;
};


struct allocate_stmt : statement {
    variable *dest;

    /* <result> = alloca <type> */
    std::string data() override {
        return string_join(
            dest->data()," = alloca ",type_map[dest->get_type()],'\n'
        );
    }

    ~allocate_stmt() override = default;
};


struct get_stmt : statement {
    inline static constexpr ssize_t NPOS = -1;
    temporary  *dst;            /* Result pointer. */
    definition *src;            /* Source pointer. */
    ssize_t     dim;            /* Dimension (index).  */
    definition *def = nullptr;  /* The index. */
    class_type *type;           /* Type of the pointer. */

    /* <result> = getelementptr <ty> ptr <ptrval> {, <ty> <idx>} */
    std::string data() override {
        std::string __suffix;
        if(def) __suffix = ", i32" + def->data();
        return string_join(
            dst->data()," = getelementptr ",type->name,
            " ptr ",src->data(),__suffix
        );
    }
};


struct phi_stmt : statement {
    using pair_t = struct {
        definition *value;
        label_type *label;
    };

    variable *dest;             /* Result.     */
    std::vector <pair_t> cond;  /* Conditions. */

    /* <result> = phi <ty> [ <value,label> ],...... */
    std::string data() override {
        std::vector <std::string> __tmp;

        __tmp.reserve(cond.size() + 2);
        __tmp.push_back(string_join(
            dest->data()," =  phi ",
            type_map[dest->get_type()])
        );
        for(auto [value,label] : cond) {
            __tmp.push_back(string_join(
                (label == cond.front().label ? " [ " : " ,[ "),
                value->data()," , ",label->label," ]"
            ));
        } __tmp.push_back("\n");

        return string_join_array(__tmp.begin(),__tmp.end());
    }

    ~phi_stmt() override = default;
};


}