#pragma once

#include "utility.h"
#include "IRnode.h"
#include "ASMregister.h"

#include <string>
#include <cstring>
#include <vector>

namespace dark::ASM {

struct node;

/* Address type. */
struct address_type {
    virtual std::string data() const = 0;
    virtual ~address_type() = default;
};


struct label_type {
    std::string name;
    std::string label() const { return 'L' + name; }
};

struct block : label_type {
    std::vector <node  *> expr;

    void emplace_new(node *__n) { expr.emplace_back(__n); }
};


struct function : label_type {
    size_t max_arg_size = -1;   /* Max argument count of functions called. */
    size_t virtual_size = 0;    /* Count of virtual variables. */
    std::map <IR::variable *, size_t> var_map;

    std::vector <block *> stmt;

    void update_size(IR::function *__func) {
        max_arg_size = std::max(max_arg_size, __func->args.size());
    }

    void emplace_new(block *__b) { stmt.emplace_back(__b); }
    void emplace_var(IR::variable *__var) { var_map.emplace(__var, var_map.size()); }
};



struct node {
    virtual std::string data() const = 0;
    virtual ~node() = default;
};


struct arith_expr : node {
    using c_string = char [8];
    enum : unsigned char {
        ADD  = 0,
        SUB  = 1,
        MUL  = 2,
        DIV  = 3,
        REM  = 4,
        SLL  = 5,
        SRA  = 6,
        AND  = 7,
        OR_  = 8,
        XOR  = 9,
    } op;
    inline static constexpr c_string str[10] = {
        [ADD]   = {'a','d','d'},
        [SUB]   = {'s','u','b'},
        [MUL]   = {'m','u','l'},
        [DIV]   = {'d','i','v'},
        [REM]   = {'r','e','m'},
        [SLL]   = {'s','l','l'},
        [SRA]   = {'s','h','a'},
        [AND]   = {'a','n','d'},
        [OR_]   = {'o','r'},
        [XOR]   = {'x','o','r'},
    };

    value_type  *lval; /* Left  value. */
    value_type *rval; /* Right value. */
    register_  *dest; /* Result register. */

    explicit arith_expr(decltype (op)    __op,
                        value_type      *__lval,
                        value_type      *__rval,
                        register_       *__dest)
        : op(__op), lval(__lval), rval(__rval), dest(__dest) {}

    std::string data() const override {
        std::string buf = str[op];
        if(dynamic_cast <immediate *> (lval)
        || dynamic_cast <immediate *> (rval)) buf += "i";
        return string_join(buf, ' ', lval->data(),", ", rval->data());
    }

    ~arith_expr() override = default;
};


struct branch_expr : node {
    using c_string = char [8];
    enum : unsigned char {
        EQ = 0,
        NE = 1,
        GE = 2,
        LT = 3,
    } op;

    inline static constexpr c_string str[4] = {
        [EQ] = {'b','e','q'},
        [NE] = {'b','n','e'},
        [GE] = {'b','g','e'},
        [LT] = {'b','l','t'},
    };

    value_type *lval; /* Left  value. */
    value_type *rval; /* Right value. */
    block     *dest; /* Destination block. */

    explicit branch_expr(decltype (op) __op,
                         value_type *__lval,
                         value_type *__rval,
                         block      *__dest)
        : op(__op), lval(__lval), rval(__rval), dest(__dest) {}

    std::string data() const override {
        return string_join(str[op],' ', lval->data(),", ", rval->data(),", ", dest->label());
    }

    ~branch_expr() override = default;
};


/* Special expr for bool types.  */
struct bool_expr   : node {
    value_type *cond; /* Condition. */
    block      *dest; /* Destination block. */

    explicit bool_expr(value_type *__cond, block *__dest)
        : cond(__cond), dest(__dest) {}

    std::string data() const override {
        return string_join("bnez ", cond->data(),", ",dest->label());
    }

    ~bool_expr() override = default;
};

struct set_less_expr : node {
    value_type *lval; /* Left  value. */
    value_type *rval; /* Right value. */
    register_  *dest; /* Result register. */

    explicit set_less_expr(value_type *__lval,
                           immediate  *__rval,
                           register_  *__dest)
        noexcept : lval(__lval), rval(__rval), dest(__dest) {}
    
    std::string data() const override {
        std::string buf = "slt";
        if(dynamic_cast <immediate *> (rval)) buf += 'i';
        return string_join(buf,' ', dest->data(),", ", lval->data(),", ", rval->data());
    }

    ~set_less_expr() override = default;
};

/**
 * @brief Not expression for booleans only. 
 * 
 */
struct not_expr : node {
    value_type *rval;
    register_  *dest;

    explicit not_expr(register_ *__rval, register_ *__dest)
        noexcept : rval(__rval), dest(__dest) {}

    std::string data() const override {
        return string_join("sltiu ", dest->data(),", ", rval->data(),", 1");
    }

    ~not_expr() override = default;
};


struct load_memory : node {
    using c_string = char [8];
    enum : unsigned char {
        BYTE = 0,
        HALF = 1,
        WORD = 2,
    } op;
    inline static constexpr c_string str[3] = {
        [BYTE] = {'l','b'},
        [HALF] = {'l','h'},
        [WORD] = {'l','w'},
    };

    address_type  *addr;    /* Address register. */
    register_     *dest;    /* Result  register. */

    explicit load_memory(decltype (op) __op,
                         address_type *__addr,
                         register_    *__dest)
        : op(__op),addr(__addr), dest(__dest) {}

    std::string data() const override {
        return string_join(str[op], ' ', dest->data(),", ", addr->data());
    }

    ~load_memory() override = default;
};



struct store_memory : node {
    using c_string = char [8];
    enum : unsigned char {
        BYTE = 0,
        HALF = 1,
        WORD = 2,
    } op;
    inline static constexpr c_string str[3] = {
        [BYTE] = {'s','b'},
        [HALF] = {'s','h'},
        [WORD] = {'s','w'},
    };

    register_     *from;  /* Source register. */
    address_type  *addr;  /* Address register. */

    explicit store_memory(decltype (op) __op,
                          register_    *__src,
                          address_type *__addr)
        : op(__op), from(__src), addr(__addr) {}

    std::string data() const override {
        return string_join(str[op],' ',from->data(),", ",addr->data());
    }

    ~store_memory() override = default;
};


/* Load a symbol's address to register. */
struct load_symbol : node {
    register_ *dst; /* Destination register. */
    symbol    *sym;
    explicit load_symbol(register_ *__dst, symbol *__sym)
        : dst(__dst), sym(__sym) {}

    std::string data() const override {
        return string_join("la ", dst->data(),", ", sym->data());
    }

    ~load_symbol() override = default;
};

/* Load an immediate number. */
struct load_immediate : node {
    register_ *dst; /* Destination register. */
    immediate *src; /* Source immediate. */

    explicit load_immediate(register_ *__dst, immediate *__src)
        : dst(__dst), src(__src) {}

    std::string data() const override {
        return string_join("li ", dst->data(),", ", src->data());
    }
    ~load_immediate() override = default;
};


struct call_expr : node {
    function *func; /* Function name. */

    explicit call_expr(function *__func) : func(__func) {}

    std::string data() const override { return string_join("call ", func->label()); }

    ~call_expr() override = default;
};

struct move_expr : node {
    register_ *dst; /* Destination register. */
    register_ *src; /* Source register. */

    explicit move_expr(register_ *__dst, register_ *__src)
        : dst(__dst), src(__src) {}

    std::string data() const override {
        return string_join("mv ", dst->data(),", ", src->data());
    }
    ~move_expr() override = default;
};


struct jump_expr : node {
    block *label; /* Label name. */

    explicit jump_expr(block *__label) : label(__label) {}

    std::string data() const override { return string_join("j ", label->label()); }

    ~jump_expr() override = default;
};


struct return_expr : node {
    function *func; /* Function name. */

    explicit return_expr(function *__func) : func(__func) {}

    std::string data() const override { return string_join("ret"); }
};



struct stack_address final : address_type {
    function    *func; /* Function . */
    IR::variable *var; /* Variable. */

    explicit stack_address(function *__func, IR::variable *__var)
        : func(__func), var(__var) {}
    
    std::string data() const override {
        return string_join(var->name, "(", func->label(), ")");
    }
    
    ~stack_address() override = default;
};


struct global_address final : address_type {
    std::string name; /* Variable name. */

    explicit global_address(std::string_view __name) : name(__name) {}

    std::string data() const override { return name; }

    ~global_address() override = default;
};


struct register_address final : address_type {
    register_ *reg;    /* Register. */
    immediate *offset; /* Offset. */

    explicit register_address(register_ *__reg, immediate *__offset)
        : reg(__reg), offset(__offset) {}

    std::string data() const override { return reg->data(); }

    ~register_address() override = default;
};


}
