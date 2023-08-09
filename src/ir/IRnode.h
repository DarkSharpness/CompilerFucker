#pragma once

#include "scope.h"
#include "IRtype.h"

#include <string>
#include <vector>


namespace dark::IR {

struct node {
    virtual std::string data() const = 0;
    virtual ~node() = default;
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
    virtual wrapper get_value_type() const = 0;
    wrapper get_point_type() const { return --get_value_type(); };
};

/* Non literal type. (Variable / Temporary) */
struct non_literal : definition {
    std::string name; /* Unique name.  */
    wrapper     type; /* Type wrapper. */
    wrapper get_value_type() const override final { return type; }
    std::string       data() const override final { return name; }
};

/* Literal constants. */
struct literal : definition {
    /* Special function used in global variable initialization. */
    virtual std::string type_data() const = 0;
};

/* Variables are pointers to value. */
struct variable : non_literal {
    ~variable() override = default;
};

/* Temporaries! */
struct temporary : non_literal {
    ~temporary() override = default;
};

struct string_constant : literal {
    std::string  context;
    cstring_type    type;
    explicit string_constant(const std::string &__ctx) : context(__ctx),type({__ctx.length()}) {}
    wrapper get_value_type() const override { return wrapper {&type,0}; }
    std::string  type_data() const override {
        return string_join("private unnamed_addr constant ",type.name()); 
    }
    std::string data() const override {
        std::string __ans = "c\"";
        for(char __p : context) {
            switch(__p) {
                case '\n': __ans += "\\0A"; break;
                case '\"': __ans += "\\22"; break;
                case '\\': __ans += "\\5C"; break;
                default: __ans.push_back(__p);
            }
        }
        __ans.push_back('\\');
        __ans.push_back('0');
        __ans.push_back('0');
        __ans.push_back('\"');
        return __ans;
    }
    ~string_constant() override = default;
};

struct pointer_constant : literal {
    variable *var;
    explicit pointer_constant(variable *__ptr) : var(__ptr) {}
    std::string  type_data() const override { return "global ptr"; }
    wrapper get_value_type() const override { return var ? ++var->type : wrapper {&__null_class__,0}; }
    std::string  data()      const override { return var ? var->name : "null"; }
    ~pointer_constant() override = default;
};

struct integer_constant : literal {
    int value;
    /* Must initialize with a value. */
    explicit integer_constant(int __v) : value(__v) {}
    std::string  type_data() const override { return "global i32"; }
    wrapper get_value_type() const override { return {&__integer_class__,0}; }
    std::string  data()      const override { return std::to_string(value); }
    ~integer_constant() override = default;
};

struct boolean_constant : literal {
    bool value;
    explicit boolean_constant(bool __v) : value(__v) {}
    std::string  type_data() const override { return "global i1"; }
    wrapper get_value_type() const override { return {&__boolean_class__,0}; }
    std::string  data()      const override { return value ? "true" : "false"; }
    ~boolean_constant() override = default;
};


struct function {
  private:
    size_t for_count   = 0; /* This is used to help generate IR. */
    size_t cond_count  = 0; /* This is used to help generate IR. */
    size_t while_count = 0; /* This is used to help generate IR. */
    std::vector <temporary *> temp; /* Real temporary holder. */
    std::map <std::string,size_t> count; /* Count of each case. */

  public:
    std::string name; /* Function name. */

    std::vector < variable  *> args; /* Argument list.   */
    std::vector <block_stmt *> stmt; /* Body data.       */
    wrapper                    type; /* Return type. */

    std::string create_label(const std::string &__name)
    { return string_join(__name,'-',std::to_string(count[__name]++)); }

    /* Create a temporary and return pointer to it. */
    temporary *create_temporary(wrapper __type) {
        return create_temporary(__type,"-");
    }

    /* Create a temporary with given name and return pointer to __temp. */
    temporary *create_temporary(wrapper __type,const std::string &__name) {
        auto *__temp = temp.emplace_back(new temporary);
        __temp->name = '%' + __name + std::to_string(count[__name]++);
        __temp->type = __type;
        return __temp;
    }

    /* Create a temporary with given name (no suffix) and return pointer to __temp. */
    temporary *create_temporary_no_suffix(wrapper __type,const std::string &__name) {
        auto *__temp = temp.emplace_back(new temporary);
        __temp->name = '%' + __name;
        __temp->type = __type;
        return __temp;
    }

    /**
     * @brief Return pointer to the first temporary of the function.
     * If member function, then current temporary is 'this' pointer.
     * This only works when this pointer is preloaded.
    */
    temporary *front_temporary() { return temp.front(); }

    /* define <ResultType> @<FunctionName> (...) {...} */
    std::string data() const {
        std::string __arg; /* Arglist. */
        for(auto &__p : args) {
            if(&__p != args.data())
                __arg += ',';
            __arg += __p->get_value_type().name();
            __arg += ' ';
            __arg += __p->data();
        }

        std::vector <std::string> __tmp;

        __tmp.reserve(stmt.size() + 2);
        __tmp.push_back(string_join(
            "define ",type.name()," @",name,
            "(",std::move(__arg),") {\n"
        ));
        for(auto __p : stmt) __tmp.push_back(__p->data());
        __tmp.push_back(string_join("}\n\n"));

        return string_join_array(__tmp.begin(),__tmp.end());
    };

    /* Declaration. */
    std::string declare() const {
        std::string __arg; /* Arglist. */
        for(auto &__p : args) {
            if(&__p != args.data())
                __arg += ',';
            __arg += __p->get_value_type().name();
        }

        return string_join(
            "declare ",type.name()," @",name,
            "(",std::move(__arg),")\n"
        );
    }


    /* Add one statement to the last block. */
    void emplace_new(statement *__stmt)
    { return stmt.back()->emplace_new(__stmt); }

    /* Opens a new block. */
    void emplace_new(block_stmt *__stmt)
    { return stmt.push_back(__stmt); }

};


struct compare_stmt : statement {
    using c_string = char[4];
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
    std::string data() const override {
        runtime_assert("Invalid compare expression!",
            lvar->get_value_type() == rvar->get_value_type(),
            dest->get_value_type().name() == "i1"
        );
        return string_join(
            dest->data()," = icmp "
            ,str[op],' ',lvar->get_value_type().name(),' '
            ,lvar->data(),", ",rvar->data(),'\n'
        );
    };
};


struct binary_stmt : statement {
    using c_string = char [8];
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
    std::string data() const override {
        runtime_assert("Invalid binary expression!",
            lvar->get_value_type() ==
                rvar->get_value_type(),
            lvar->get_value_type() ==
                dest->get_value_type()
        );
        return string_join(
            dest->data()," = ",str[op],' ',
            dest->get_value_type().name(),' '
            ,lvar->data(),", ",rvar->data(),'\n'
        );
    };

    ~binary_stmt() override = default;
};


/* Unconditional jump. */
struct jump_stmt : statement {
    block_stmt *dest;

    /* br label <dest> */
    std::string data() const override {
        return string_join("br label %",dest->label,'\n');
    }
};


/* Conditional branch. */
struct branch_stmt : statement {
    definition *cond;  /* The condition. */
    block_stmt *br[2]; /* Label with a name. */

    /* br i1 <cond>, label <iftrue>, label <if_false> */
    std::string data() const override {
        runtime_assert("Branch condition must be boolean type!",
            cond->get_value_type().name() == "i1"
        );
        return string_join(
            "br i1 ",cond->data(),
            ", label %",br[0]->label,
            ", label %",br[1]->label,
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
    std::string data() const override {
        std::string __arg; /* Arglist. */
        for(auto &__p : args) {
            if(&__p != args.data())
                __arg += ',';
            __arg += __p->get_value_type().name();
            __arg += ' ';
            __arg += __p->data();
        }
        std::string __prefix;
        if(func->type.name() != "void")
            __prefix = dest->data() + " = "; 
        return string_join(
            __prefix, "call ",
           func->type.name()," @",func->name,
            "(",std::move(__arg),")\n"
        );
    }
    ~call_stmt() override = default;
};

struct load_stmt : statement {
    non_literal *src;
    temporary   *dst;

    /* <result> = load <type>, ptr <pointer>  */
    std::string data() const override {
        runtime_assert("Invalid load statement",
            src->get_point_type() == dst->get_value_type()
        );
        return string_join(
            dst->data(), " = load ",
            dst->get_value_type().name(),", ptr ",src->data(),'\n'
        );
    }

    ~load_stmt() = default;
};


struct store_stmt : statement {
    definition  *src;
    non_literal *dst;

    /* store <type> <value>, ptr <pointer>  */
    std::string data() const override {
        runtime_assert("Invalid load statement!",
            src->get_value_type() == dst->get_point_type()
        );
        return string_join(
            "store ",src->get_value_type().name(),' ',
            src->data(),", ptr ",dst->data(),'\n'
        );
    }

    ~store_stmt() override = default;
};


struct return_stmt : statement {
    function   *func = nullptr;
    definition *rval = nullptr;

    std::string data() const override {
        runtime_assert("Invalid return statement!",
            !rval || func->type == rval->get_value_type(),
             rval || func->type.name() == "void"
        );
        if(!rval) return "ret void\n";
        else {
            return string_join(
                "ret ",func->type.name(),' ',rval->data(),'\n'
            );
        }
    }
    ~return_stmt() override = default;
};


struct allocate_stmt : statement {
    variable *dest; /* Destination must be local! */

    /* <result> = alloca <type> */
    std::string data() const override {
        return string_join(
            dest->data()," = alloca ",dest->get_point_type().name(),'\n'
        );
    }

    ~allocate_stmt() override = default;
};


struct get_stmt : statement {
    inline static constexpr ssize_t NPOS = -1;
    temporary  *dst;            /* Result pointer. */
    definition *src;            /* Source pointer. */
    definition *idx;            /* Index of the array. */
    size_t      mem;            /* Index of member. */

    /* <result> = getelementptr <ty>, ptr <ptrval> {, <ty> <idx>} */
    std::string data() const override {
        std::string __suffix;
        if(mem != NPOS) __suffix = ", i32 " + std::to_string(mem);
        else runtime_assert("Invalid get statement!",
            idx->get_value_type().name() == "i32",
            dst->get_value_type() == src->get_value_type()
        );
        return string_join(
            dst->data()," = getelementptr ",src->get_point_type().name(),
            ", ptr ",src->data(), ", i32 ",idx->data(),__suffix,'\n'
        );
    }
};


struct phi_stmt : statement {
    using pair_t = struct {
        definition *value;
        block_stmt *label;
    };

    temporary *dest;             /* Result.     */
    std::vector <pair_t> cond;  /* Conditions. */

    /* <result> = phi <ty> [ <value,label> ],...... */
    std::string data() const override {
        std::vector <std::string> __tmp;

        __tmp.reserve(cond.size() + 2);
        __tmp.push_back(string_join(
            dest->data()," = phi ", dest->get_value_type().name())
        );
        for(auto [value,label] : cond) {
            runtime_assert("Invalid phi statement!",
                dest->get_value_type() == value->get_value_type()
            );
            __tmp.push_back(string_join(
                (label == cond.front().label ? " [ " : " , [ "),
                value->data()," , %",label->label," ]"
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