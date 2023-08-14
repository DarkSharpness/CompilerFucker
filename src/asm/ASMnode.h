#pragma once

#include "utility.h"
#include "IRnode.h"
#include "ASMregister.h"

#include <string>
#include <cstring>
#include <vector>

namespace dark::ASM {

struct node;

struct label_type {
    std::string name;
    std::string label() const {
        return string_join("L", name);
    }
};

struct block : label_type {
    std::vector <node  *> expr;

    void emplace_new(node *__n) { expr.emplace_back(__n); }
};


struct function : label_type {
    std::vector <block *> stmt;
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

    register_ *lval; /* Left  value. */
    temporary *rval; /* Right value. */
    register_ *dest; /* Result register. */

    explicit arith_expr(decltype (op)    __op,
                        register_       *__lval,
                        low_immediate   *__rval,
                        register_       *__dest)
        : op(__op), lval(__lval), rval(__rval), dest(__dest) {}

    explicit arith_expr(decltype (op)    __op,
                        low_immediate   *__lval,
                        register_       *__rval,
                        register_       *__dest)
        : op(__op), lval(__rval), rval(__lval), dest(__dest) {}

    explicit arith_expr(decltype (op)    __op,
                        register_       *__lval,
                        register_       *__rval,
                        register_       *__dest)
        : op(__op), lval(__lval), rval(__rval), dest(__dest) {}

    std::string data() const override {
        std::string buf = str[op];
        if(dynamic_cast <immediate *> (rval)) buf += "i";
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

    register_ *lval; /* Left  value. */
    register_ *rval; /* Right value. */
    block     *dest; /* Destination block. */

    explicit branch_expr(decltype (op) __op,
                          register_  *__lval,
                          register_  *__rval,
                          block      *__dest)
        : op(__op), lval(__lval), rval(__rval), dest(__dest) {}

    std::string data() const override {
        return string_join(str[op],' ', lval->data(),", ", rval->data());
    }

    ~branch_expr() override = default;
};


struct set_less_expr : node {
    register_ *lval; /* Left  value. */
    temporary *rval; /* Right value. */
    register_ *dest; /* Result register. */

    explicit set_less_expr(register_     *__lval,
                           low_immediate *__rval,
                            register_     *__dest)
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
    register_ *rval;
    register_ *dest;

    explicit not_expr(register_ *__rval, register_ *__dest)
        noexcept : rval(__rval), dest(__dest) {}

    std::string data() const override {
        return string_join("sltu ", dest->data(),", ", rval->data(),", 1");
    }

    ~not_expr() override = default;
};


struct load_expr : node {
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

    immediate   *offset;  /* Offset. */
    register_     *addr;    /* Address register. */
    register_     *dest;    /* Result  register. */

    explicit load_expr(decltype (op) __op,
                       immediate    *__offset,
                       register_    *__addr,
                       register_    *__dest)
        : op(__op), offset(__offset), addr(__addr), dest(__dest) {}

    std::string data() const override {
        return string_join(str[op], ' ', dest->data(),", ", addr->data());
    }

    ~load_expr() override = default;
};


struct load_address : node {
    std::string symbol; /* Symbom. */
    register_    *dest;
    load_address(std::string_view __symbol, register_ *__dest)
        : symbol(__symbol), dest(__dest) {}

    std::string data() const override {
        return string_join("la ", dest->data(),", ", symbol);
    }

    ~load_address() override = default;
};


struct store_expr : node {
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

    immediate   *offset;  /* Offset. */
    register_     *src;     /* Source register. */
    register_     *addr;    /* Address register. */

    explicit store_expr(decltype (op) __op,
                        immediate    *__offset,
                        register_    *__src,
                        register_    *__addr)
        : op(__op), offset(__offset), src(__src), addr(__addr) {}

    std::string data() const override {
        return string_join(str[op], ' ', src->data(),", ", addr->data());
    }

    ~store_expr() override = default;
};


struct call_expr : node {
    function *func; /* Function name. */

    explicit call_expr(function *__func) : func(__func) {}

    std::string data() const override { return string_join("call ", func->label()); }

    ~call_expr() override = default;
};


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


std::string stack_immediate::data() const {
    /* Complete it! */

};


}
