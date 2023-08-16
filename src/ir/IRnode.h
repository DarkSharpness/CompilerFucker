#pragma once

#include "scope.h"
#include "IRtype.h"
#include "IRbase.h"

#include <string>
#include <vector>


namespace dark::IR {

/* Abstract middle class. */

struct statement : node {};

struct phi_stmt;

/* Block statement is not a statement! */
struct block_stmt {
    std::string label;
    std::vector <statement *> stmt; /* Statements */

    /* Simply join all message in it together. */
    std::string data() const;
    auto emplace_new(statement *__stmt) { return stmt.push_back(__stmt); }
    phi_stmt *is_phi_block() const;
    ~block_stmt() = default;
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
    std::string data() const override;
    void accept(IRvisitorbase *v) override { return v->visitCompare(this); }
    ~compare_stmt() override = default;
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
    std::string data() const override;
    void accept(IRvisitorbase *v) override { return v->visitBinary(this); }
    ~binary_stmt() override = default;
};


/* Unconditional jump. */
struct jump_stmt : statement {
    block_stmt *dest;

    /* br label <dest> */
    std::string data() const override
    { return string_join("br label %",dest->label,'\n'); }
    void accept(IRvisitorbase *v) override { return v->visitJump(this); }
    ~jump_stmt() override = default;
};


/* Conditional branch. */
struct branch_stmt : statement {
    definition *cond;  /* The condition. */
    block_stmt *br[2]; /* Label with a name. */

    /* br i1 <cond>, label <iftrue>, label <if_false> */
    std::string data() const override;
    void accept(IRvisitorbase *v) override  { return v->visitBranch(this); }
    ~branch_stmt() override = default;
};


struct call_stmt : statement {
    function  *func;
    temporary *dest;
    std::vector <definition *> args;

    /* <result> = call <ResultType> @<FunctionName> (<argument>) */
    std::string data() const override;
    void accept(IRvisitorbase *v) override  { return v->visitCall(this); }
    ~call_stmt() override = default;
};


struct load_stmt : statement {
    non_literal *src;
    temporary   *dst;
 
    /* <result> = load <type>, ptr <pointer>  */
    std::string data() const override;
    void accept(IRvisitorbase *v) override { return v->visitLoad(this); }
    ~load_stmt() = default;
};


struct store_stmt : statement {
    definition  *src;
    non_literal *dst;

    /* store <type> <value>, ptr <pointer>  */
    std::string data() const override;
    void accept(IRvisitorbase *v) override { return v->visitStore(this); }
    ~store_stmt() override = default;
};


struct return_stmt : statement {
    function   *func = nullptr;
    definition *rval = nullptr;

    std::string data() const override;
    void accept(IRvisitorbase *v) override { return v->visitReturn(this); }
    ~return_stmt() override = default;
};


struct allocate_stmt : statement {
    variable *dest; /* Destination must be local! */

    /* <result> = alloca <type> */
    std::string data() const override;
    void accept(IRvisitorbase *v) override { return v->visitAlloc(this); }
    ~allocate_stmt() override = default;
};


struct get_stmt : statement {
    inline static constexpr ssize_t NPOS = -1;
    temporary  *dst;            /* Result pointer. */
    definition *src;            /* Source pointer. */
    definition *idx;            /* Index of the array. */
    size_t      mem;            /* Index of member. */

    /* <result> = getelementptr <ty>, ptr <ptrval> {, <ty> <idx>} */
    std::string data() const override;
    void accept(IRvisitorbase *v) override { return v->visitGet(this); }
    ~get_stmt() override = default;
};


struct phi_stmt : statement {
    using pair_t = struct {
        definition *value;
        block_stmt *label;
    };

    temporary *dest;             /* Result.     */
    std::vector <pair_t> cond;  /* Conditions. */

    /* <result> = phi <ty> [ <value,label> ],...... */
    std::string data() const override;
    void accept(IRvisitorbase *v) override { return v->visitPhi(this); }
    ~phi_stmt() override = default;
};


struct unreachable_stmt : statement {
    std::string data() const override { return "unreachable\n"; }
    void accept(IRvisitorbase *v) override { return v->visitUnreachable(this); }
    ~unreachable_stmt() override = default;
};


/* Function body. */
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

    /* Return the index of the argument. */
    size_t get_arg_index(variable *__var) const {
        for(size_t i = 0 ; i != args.size() ; ++i)
            if(args[i] == __var) return i;
        return -1;
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
    std::string data() const;
    /* Declaration. */
    std::string declare() const;

    /* Add one statement to the last block. */
    void emplace_new(statement *__stmt)
    { return stmt.back()->emplace_new(__stmt); }

    /* Opens a new block. */
    void emplace_new(block_stmt *__stmt)
    { return stmt.push_back(__stmt); }

};


/* Initialization for global variables. */
struct initialization {
    variable *dest; /* Destination.      */
    literal  *lite; /* Const expression. */

    std::string data() const {
        return string_join(
            dest->data()," = ",
            lite->type_data(),' ',lite->data()
        );
    }

    ~initialization() = default;
};



}