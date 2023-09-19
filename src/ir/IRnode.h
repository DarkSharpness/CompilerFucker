#pragma once

#include "scope.h"
#include "IRtype.h"
#include "IRbase.h"

#include <string>
#include <vector>
#include <cstdint>

namespace dark {


/**
 * @brief A wrapper that is using to help hide implement.
 */
struct hidden_impl {
  private:
    void *impl = nullptr;
  public:
    /* Set the hidden implement pointer. */
    void set_impl_ptr(void *__impl) noexcept { impl = __impl; }
    /* Set the hidden implement value. */
    template <class T>
    auto set_impl_val(T __val) noexcept
    -> std::enable_if_t <sizeof(T) <= sizeof(void *)>
    { impl = reinterpret_cast <void *> (__val); }

    /* Get the hidden implement pointer. */
    template <class T = void>
    auto get_impl_ptr() const noexcept { return static_cast <T *> (impl); }

    template <class T>
    auto get_impl_val() const noexcept
    -> std::enable_if_t <sizeof(T) <= sizeof(void *),T &>
    { return static_cast <T &> (impl); }

};

}


namespace dark::IR {

/* Abstract middle class. */

using statement = node;

struct phi_stmt;

/* Block statement is not a statement! */
struct block_stmt final : hidden_impl {
    std::string label;
    std::vector <statement *> stmt; /* Statements */

    /* Simply join all message in it together. */
    std::string data() const;
    auto emplace_new(statement *__stmt) { return stmt.push_back(__stmt); }
    std::vector <phi_stmt *> get_phi_block() const;
    /* Whether this block is unreachable. */
    bool is_unreachable() const;
    ~block_stmt() = default;
};


struct compare_stmt final : statement {
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
    temporary *get_def() const override { return dest; }
    std::vector <definition *> get_use() const override { return {lvar,rvar}; }
    void update(definition *__old, definition *__new) override {
        if(lvar == __old) lvar = __new;
        if(rvar == __old) rvar = __new;
    }

    // bool is_undefined_behavior() const override { return false; }
    ~compare_stmt() override = default;
};


struct binary_stmt final : statement {
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
    temporary *get_def() const override { return dest; }
    std::vector <definition *> get_use() const override { return {lvar,rvar}; }

    void update(definition *__old, definition *__new) override {
        if(lvar == __old) lvar = __new;
        if(rvar == __old) rvar = __new;
    }

    bool is_undefined_behavior() const override {
        if(auto *__rhs = dynamic_cast <integer_constant *> (rvar)) {
            bool __val = (op == SDIV || op == SREM) && __rhs->value == 0;
            if (__val) warning("Undefined behavior: division by zero!");
            return __val;
        } else return false;
    }

    ~binary_stmt() override = default;
};


/* Unconditional jump. */
struct jump_stmt final : statement {
    block_stmt *dest;

    /* br label <dest> */
    std::string data() const override
    { return string_join("br label %",dest->label,'\n'); }
    void accept(IRvisitorbase *v) override { return v->visitJump(this); }
    temporary *get_def() const override { return nullptr; }
    std::vector <definition *> get_use() const override { return {}; }
    void update(definition *__old, definition *__new) override {}
    // bool is_undefined_behavior() const override { return false; }
    ~jump_stmt() override = default;
};


/* Conditional branch. */
struct branch_stmt final : statement {
    definition *cond;  /* The condition. */
    block_stmt *br[2]; /* Label with a name. */

    /* br i1 <cond>, label <iftrue>, label <if_false> */
    std::string data() const override;
    void accept(IRvisitorbase *v) override { return v->visitBranch(this); }
    temporary *get_def() const override { return nullptr; }
    std::vector <definition *> get_use() const override { return {cond}; }
    void update(definition *__old, definition *__new) override {
        if(cond == __old) cond = __new;
    }
    // bool is_undefined_behavior() const override { return false; }
    ~branch_stmt() override = default;
};


struct call_stmt final : statement {
    function  *func;
    temporary *dest = nullptr; /* For the sake of safety. */
    std::vector <definition *> args;

    /* <result> = call <ResultType> @<FunctionName> (<argument>) */
    std::string data() const override;
    void accept(IRvisitorbase *v) override { return v->visitCall(this); }
    temporary *get_def() const override { return dest; }
    std::vector <definition *> get_use() const override { return args; }
    void update(definition *__old, definition *__new) override {
        for(auto &__arg : args) if(__arg == __old) __arg = __new;
    }
    // bool is_undefined_behavior() const override { return false; }
    ~call_stmt() override = default;
};


/* A tagging for type_safe! */
struct memory_stmt : statement {};


struct load_stmt final : memory_stmt {
    non_literal *src;
    temporary   *dst;
 
    /* <result> = load <type>, ptr <pointer>  */
    std::string data() const override;
    void accept(IRvisitorbase *v) override { return v->visitLoad(this); }
    temporary *get_def() const override   { return dst; }
    std::vector <definition *> get_use() const override { return {src}; }
    void update(definition *__old, definition *__new) override {
        if(src == __old) src = dynamic_cast <non_literal *> (__new);
    }

    /* Load from nullptr is hard UB! */
    bool is_undefined_behavior() const override {
        bool __val = (src == nullptr);
        if (__val) warning("Undefined behavior: load from nullptr!");
        return __val;
    }
    ~load_stmt() = default;
};


struct store_stmt final : memory_stmt {
    definition  *src;
    non_literal *dst;

    /* store <type> <value>, ptr <pointer>  */
    std::string data() const override;
    void accept(IRvisitorbase *v) override { return v->visitStore(this); }
    temporary *get_def() const override { return nullptr; }
    std::vector <definition *> get_use() const override { return {src,dst}; }
    void update(definition *__old, definition *__new) override {
        if(src == __old) src = __new;
        if(dst == __old) dst = dynamic_cast <non_literal *> (__new);
    }

    /* Store into nullptr is hard UB! */
    bool is_undefined_behavior() const override {
        bool __val = (dst == nullptr);
        if (__val) warning("Undefined behavior: store from nullptr!");
        return __val;
    }
    ~store_stmt() override = default;
};


struct return_stmt final : statement {
    function   *func = nullptr;
    definition *rval = nullptr;

    std::string data() const override;
    void accept(IRvisitorbase *v) override { return v->visitReturn(this); }
    temporary *get_def() const override { return nullptr; }
    std::vector <definition *> get_use() const override { return {rval}; }
    void update(definition *__old, definition *__new) override {
        if(rval == __old) rval = __new;
    }
    bool is_undefined_behavior() const override {
        return dynamic_cast <IR::undefined *> (rval);
    }
    ~return_stmt() override = default;
};


struct allocate_stmt final : statement {
    local_variable *dest; /* Destination must be local! */

    /* <result> = alloca <type> */
    std::string data() const override;
    void accept(IRvisitorbase *v) override { return v->visitAlloc(this); }
    temporary *get_def() const override { return nullptr; }
    std::vector <definition *> get_use() const override { return {}; }
    void update(definition *__old, definition *__new) override {}
    // bool is_undefined_behavior() const override { return false; }
    ~allocate_stmt() override = default;
};


struct get_stmt final : statement {
    inline static constexpr ssize_t NPOS = -1;
    temporary  *dst;            /* Result pointer. */
    definition *src;            /* Source pointer. */
    definition *idx;            /* Index of the array. */
    size_t      mem;            /* Index of member. */

    /* <result> = getelementptr <ty>, ptr <ptrval> {, <ty> <idx>} */
    std::string data() const override;
    void accept(IRvisitorbase *v) override { return v->visitGet(this); }
    temporary *get_def() const override { return dst; }
    std::vector <definition *> get_use() const override { return {src,idx}; }
    void update(definition *__old, definition *__new) override {
        if(src == __old) src = __new;
        if(idx == __old) idx = __new;
    }

    /* Cannot perform operation on nullptr! */
    bool is_undefined_behavior() const override {
        bool __val = __is_null_type(src->get_value_type());
        if (__val) warning("Undefined behavior: operation on nullptr!");
        return __val;
    }

    ~get_stmt() override = default;
};


struct phi_stmt final : statement {
    using pair_t = struct {
        definition *value;
        block_stmt *label;
    };

    temporary *dest;             /* Result.     */
    std::vector <pair_t> cond;  /* Conditions. */

    /* <result> = phi <ty> [ <value,label> ],...... */
    std::string data() const override;
    void accept(IRvisitorbase *v) override { return v->visitPhi(this); }
    temporary *get_def() const override { return dest; }
    std::vector <definition *> get_use() const override {
        std::vector <definition *> __use;
        for(auto __p : cond) __use.push_back(__p.value);
        return __use;
    }

    /* Update the block label of a phi statement. */
    void update(IR::block_stmt *__old , IR::block_stmt *__new) {
        for(auto &__p : cond) if(__p.label == __old)
            return static_cast <void> (__p.label = __new);
        runtime_assert("This shouldn't happen!");
    }
    void update(definition *__old, definition *__new) override {
        for(auto &__p : cond) if(__p.value == __old) __p.value = __new;
    }
    // bool is_undefined_behavior() const override { return false; }
    ~phi_stmt() override = default;
};


struct unreachable_stmt final : statement {
    unreachable_stmt(unreachable_stmt *) {}
    std::string data() const override { return "unreachable\n"; }
    void accept(IRvisitorbase *v) override { return v->visitUnreachable(this); }
    temporary *get_def() const override { return nullptr; }
    std::vector <definition *> get_use() const override { return {}; }
    void update(definition *__old, definition *__new) override {}
    bool is_undefined_behavior() const override { return true; }
    ~unreachable_stmt() override = default;
};


/* Return the only object of this class. */
inline unreachable_stmt *create_unreachable() {
    static unreachable_stmt __unreachable(nullptr);
    return &__unreachable;
}


/* Function body. */
struct function : hidden_impl {
  private:
    size_t for_count   = 0; /* This is used to help generate IR. */
    size_t cond_count  = 0; /* This is used to help generate IR. */
    size_t while_count = 0; /* This is used to help generate IR. */
    std::vector <temporary *> temp; /* Real temporary holder. */
    std::map <std::string,size_t> count; /* Count of each case. */

  public:
    static constexpr uint8_t NONE = 0b00;
    static constexpr uint8_t IN   = 0b01;
    static constexpr uint8_t OUT  = 0b10;
    static constexpr uint8_t INOUT= 0b11;
 
    std::string name;            /* Function name. */
    bool    is_builtin  = false; /* Builtin function. */
    uint8_t inout_state = NONE;  /* Inout state.      */

    std::vector <function_argument *> args; /* Argument list.   */
    std::vector     <block_stmt *>    stmt; /* Body data.       */
    wrapper                           type; /* Return type. */

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

    /* Whether the function is side effective. */

    size_t is_side_effective() const;
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

    /* An unreachable function. */
    bool is_unreachable() const { return stmt.front()->is_unreachable(); }

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