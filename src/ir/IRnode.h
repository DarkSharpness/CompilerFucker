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


struct node {
    virtual std::string data() = 0;
    virtual ~node() = default;
};


/* This is a special type marker. */
struct class_type {
    std::string name; /* Class names contains '%' || Others contains only name. */
    std::vector <typeinfo>    layout; /* Variable typeinfo. */
    std::vector <std::string> member; /* Member variables. */

    size_t index(std::string_view __view) const {
        for(size_t i = 0 ; i != member.size() ; ++i)
            if(member[i] == __view) return i;
        throw dark::error("This should never happen! No such Member!");
    }

    bool is_builtin() const noexcept { return name[0] != '%'; }
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
        for(auto __p : stmt) __tmp.push_back("    " + __p->data());
        __tmp.push_back("\n");

        return string_join_array(__tmp.begin(),__tmp.end());
    }

    void emplace_new(statement *__stmt)
    { return stmt.push_back(__stmt); }
};


/* A definition can be variable / literal */
struct definition : node {
    virtual typeinfo get_type() = 0;
};

/* Variables or temporaries. */
struct value_type : definition {
    std::string name; /* Unique name. */
    typeinfo    type; /* Actual type. */
    typeinfo get_type() override final { return type; }
    std::string  data() override final { return name; }
};


/* Pure variables. */
struct variable  : value_type {
    ~variable() override = default;
};

struct temporary : value_type {
    ~temporary() override = default;
};

/* Literal constants. */
struct literal    : definition {
    virtual std::string type_data() = 0;
};

struct string_constant : literal {
    std::string context;
    string_constant(const std::string &__ctx) : context(__ctx) {}
    std::string type_data() override {
        return
            string_join(
                "private unnamed_addr constant [",
                std::to_string(context.length() - 1), /* This length is for fun only. */
                " x i8]"
            );
    }
    typeinfo get_type() override { return typeinfo::PTR; }
    std::string  data() override { return string_join('c',context); }
    ~string_constant()  override = default;
};

struct pointer_constant : literal {
    variable *var;
    pointer_constant(variable *__ptr) : var(__ptr) {}
    std::string type_data() override { return "ptr"; }
    typeinfo get_type() override { return typeinfo::PTR; }
    std::string  data() override { return var ? var->name : "null"; }
    ~pointer_constant()    override = default;
};

struct integer_constant : literal {
    int value;
    /* Must initialize with a value. */
    explicit integer_constant(int __v) : value(__v) {}
    std::string type_data() override { return "i32"; }
    typeinfo get_type() override { return typeinfo::I32; }
    std::string  data() override { return std::to_string(value); }
    ~integer_constant() override = default;
};

struct boolean_constant : literal {
    bool value;
    explicit boolean_constant(bool __v) : value(__v) {}
    std::string type_data() override { return "i1"; }
    typeinfo get_type() override { return typeinfo::I1; }
    std::string  data() override { return value ? "true" : "false"; }
    ~boolean_constant() override = default;
};


struct function {
  private:
    std::vector <temporary> vec; /* Real data holder. */
    size_t temp_count  = 0; /* This is used to help generate IR. */
    size_t cond_count  = 0; /* This is used to help generate IR. */
    size_t for_count   = 0; /* This is used to help generate IR. */
    size_t while_count = 0; /* This is used to help generate IR. */

  public:
    std::string name; /* Function name. */

    std::vector <variable   *> args; /* Argument list.   */
    std::vector <block_stmt *> stmt; /* Body data.       */
    typeinfo                   type; /* Return type. */

    std::string create_branch() {
        return "if-" + std::to_string(cond_count++);
    }

    std::string create_for() {
        return "for-" + std::to_string(for_count++); 
    }

    std::string create_while() {
        return "while-" + std::to_string(for_count++); 
    }


    /* Create a temporary and return pointer to it. */
    temporary *create_temporary(typeinfo __type) {
        return create_temporary(__type,std::to_string(temp_count++));
    }

    /* Create a temporary with given name and return pointer to it. */
    temporary *create_temporary(typeinfo __type,const std::string &__name) {
        temporary __temp;
        __temp.name = '%' + __name;
        __temp.type = __type;
        vec.push_back(__temp);
        return &vec.back();
    }



    /* define <ResultType> @<FunctionName> (...) {...} */
    std::string data() {
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

    /* Add one statement to the last block. */
    void emplace_new(statement *__stmt)
    { return stmt.back()->emplace_new(__stmt); }

    /* Opens a new block. */
    void emplace_new(block_stmt *__stmt)
    { return stmt.push_back(__stmt); }

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
        return string_join(
            dest->data()," = icmp "
            ,str[op],' ',type_map[lvar->get_type()],' '
            ,lvar->data(),", ",rvar->data(),'\n'
        );
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
        return string_join(
            dest->data()," = ",str[op]
            ," i32 ",lvar->data(),", ",rvar->data(),'\n'
        );
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
            '\n'
        );
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

struct load_stmt : statement {
    value_type *src;
    temporary  *dst;

    /* <result> = load <type>, ptr <pointer>  */
    std::string data() override {
        return string_join(
            dst->data(), " = load ",
            type_map[dst->get_type()],", ptr ",src->data(),'\n'
        );
    }

    ~load_stmt() = default;
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
    definition *rval = nullptr;

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
    variable *dest; /* Destination must be local! */

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
    size_t      idx;            /* The index. */
    class_type *type;           /* Type of the pointer. */

    /* <result> = getelementptr <ty> ptr <ptrval> {, <ty> <idx>} */
    std::string data() override {
        std::string __suffix;
        if(idx != NPOS) __suffix = ", i32" + std::to_string(idx);
        return string_join(
            dst->data()," = getelementptr ",type->name,
            " ptr ",src->data(),__suffix
        );
    }
};


struct phi_stmt : statement {
    using pair_t = struct {
        definition *value;
        block_stmt *label;
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


/* Initialization for global variables. */
struct initialization {
    variable *dest; /* Destination.      */
    literal  *lite; /* Const expression. */

    std::string data() {
        return string_join(
            dest->data()," = ",
            lite->type_data(),' ',lite->data()
        ); 
    }

    ~initialization() = default;
};



}