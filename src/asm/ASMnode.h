#pragma once

#include "utility.h"
#include "IRnode.h"
#include "ASMregister.h"

#include <string>
#include <cstring>
#include <vector>

namespace dark::ASM {

struct ASMvisitorbase;

struct node {
    virtual std::string data() const = 0;
    virtual void accept(struct ASMvisitorbase* __v) = 0;
    virtual ~node() = default;
};
struct arith_expr;
struct branch_expr;
struct slt_expr;
struct eq_expr;
struct not_expr;
struct bool_expr;
struct load_memory;
struct store_memory;
struct load_symbol;
struct load_immediate;
struct call_expr;
struct move_expr;
struct jump_expr;
struct return_expr;

struct register_address;


struct ASMvisitorbase {
    void visit(node *__n) { __n->accept(this); }
    virtual void visitArithExpr(arith_expr *) = 0;
    virtual void visitBranchExpr(branch_expr *) = 0;
    virtual void visitSltExpr(slt_expr *) = 0;
    virtual void visitEqExpr(eq_expr *) = 0;
    virtual void visitNotExpr(not_expr *) = 0;
    virtual void visitBoolExpr(bool_expr *) = 0;
    virtual void visitLoadMemory(load_memory *) = 0;
    virtual void visitStoreMemory(store_memory *) = 0;
    virtual void visitLoadSymbol(load_symbol *) = 0;
    virtual void visitLoadImmediate(load_immediate *) = 0;
    virtual void visitCallExpr(call_expr *) = 0;
    virtual void visitMoveExpr(move_expr *) = 0;
    virtual void visitJumpExpr(jump_expr *) = 0;
    virtual void visitReturnExpr(return_expr *) = 0;

    virtual ~ASMvisitorbase() = default;
};


struct register_allocator {
    /* Position information. */
    inline constexpr static size_t NPOS = size_t(-1);

    struct store_info {
        physical_register *reg; /* Physical register. */
        size_t      pos = NPOS; /* Store position. */
        bool is_store() const noexcept { return pos != NPOS; }
    };

    struct load_info {
        physical_register *reg; /* Physical register. */
        size_t     lpos = NPOS; /* Load position */
        size_t     spos = NPOS; /* Store position */

        bool is_load()  const noexcept { return lpos != NPOS; }
        bool is_store() const noexcept { return spos != NPOS; }
    };

    /* Return the register with given position to store. */
    virtual store_info allocate(virtual_register *) = 0;
    /* Return the register with given position to store and load. */
    virtual std::vector <load_info> access(std::vector <virtual_register *>) = 0;
    /* Return the registers with given position to store.  */
    virtual std::vector <store_info> call() = 0;

    /* Retrieve one register. */
    virtual void  deallocate(virtual_register *__reg) = 0;
    virtual size_t max_size() const = 0;
    virtual ~register_allocator() = default;
};


/* Address type. */
struct address_type {
    virtual std::string data() const = 0;
    virtual ~address_type() = default;
};


struct block {
    inline static size_t label_count = 0;
    std::string name;
    explicit block (std::string __str) :
        name(std::to_string(label_count++) + '.' + __str) {}

    std::string label() const { return ".L." + name; }
    std::vector <node  *> expr;

    void emplace_new(node *__n) { expr.emplace_back(__n); }

    std::string data() const {
        std::vector <std::string> buf;
        buf.reserve(expr.size() * 2 + 2);
        buf.push_back(label() + ":");
        for(auto __p : expr) {
            buf.push_back("\n    ");
            buf.push_back(__p->data());
        }
        buf.push_back("\n");
        return string_join_array(buf.begin(), buf.end());
    }

};


struct function {
    std::string name; /* Name of the function. */
    ssize_t max_arg_size = -1;  /* Max argument count of functions called. */
    size_t  vir_size     = 0;   /* Count of virtual variables (Meaningless counter.). */
    size_t  var_size     = 0;   /* Count of all variables. */
    size_t  stk_size     = 0;   /* Count of stack variables. */

    /* Virtual register allocator. */
    register_allocator *__alloc = nullptr;

    /* Mapping of an IR::variable to its address in stack. */
    std::map <IR::variable *,ssize_t> var_map;
    std::vector <block *> stmt;

    /* Emplace a function argument. */
    void emplace_arg(const std::vector <IR::variable *> &args) {
        /* Set the address for the variable. */
        for(size_t i = 8 ; i < args.size() ; ++i)
            var_map[args[i]] = (ssize_t)(7 - i);
    }

    /* Update max function size. */
    void update_size(IR::function *__func) {
        max_arg_size = std::max(max_arg_size,(ssize_t)__func->args.size());
    }

    /* Whether calling other functions. */
    bool is_calling() const { return max_arg_size != size_t(-1); }
    /* Max stack space reserved for arguments. */
    size_t arg_size() const { return (ssize_t)max_arg_size > 8 ? max_arg_size - 8 : 0; }

    /* Emplace a new node. */
    void emplace_new(block *__b) { stmt.emplace_back(__b); }

    /* Emplace a new variable. */
    void emplace_var(IR::variable *__var)
    { var_map.emplace(__var,(ssize_t) (var_size++)); }

    /* Calculate the stack space. */
    void init_function() {
        stk_size = (
            arg_size() +            /* Function arguments for function called. */
            (__alloc ? __alloc->max_size() : 0) +   /* Temporay values.  */
            var_size +              /* Local variables */
            is_calling()            /* Return address. */
        ) * 4;

        /* Aligned to 16 byte. */
        if(stk_size % 16 != 0) stk_size = (stk_size / 16) * 16 + 16;
    }

    /* Local variable. */
    size_t get_variable_offset(IR::variable *__var) const {
        return stk_size - (var_map.at(__var) + 1 + is_calling()) * 4; 
    }

    std::string data() const {
        std::vector <std::string> buf;
        buf.reserve(stmt.size() + 5);
        buf.push_back(string_join(
            "    .globl ", name,'\n',
            name,":\n" 
        ));

        if(is_calling()) {
            buf.push_back("    sw ra, -4(sp)\n    ");
        }
        if(stk_size)
            buf.push_back(string_join(
                "addi sp, sp, -", std::to_string(stk_size),'\n'
            ));

        for(auto __p : stmt)
            buf.push_back(__p->data());

        return string_join_array(buf.begin(), buf.end());
    }
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
    value_type  *rval; /* Right value. */
    register_   *dest; /* Result register. */

    explicit arith_expr(decltype (op)    __op,
                        value_type      *__lval,
                        value_type      *__rval,
                        register_       *__dest)
        : op(__op), lval(__lval), rval(__rval), dest(__dest) {}

    std::string data() const override {
        std::string buf = str[op];
        if(dynamic_cast <immediate *> (lval)
        || dynamic_cast <immediate *> (rval)) buf += "i";
        return string_join(buf,' ',dest->data(),", ", lval->data(),", ", rval->data());
    }

    void accept(ASMvisitorbase *__v) override { __v->visitArithExpr(this); }

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
    block      *dest; /* Destination block. */

    explicit branch_expr(decltype (op) __op,
                         value_type *__lval,
                         value_type *__rval,
                         block      *__dest)
        : op(__op), lval(__lval), rval(__rval), dest(__dest) {}

    std::string data() const override {
        return string_join(str[op],' ', lval->data(),", ", rval->data(),", ", dest->label());
    }


    void accept(ASMvisitorbase *__v) override { __v->visitBranchExpr(this); }

    ~branch_expr() override = default;
};


struct slt_expr : node {
    value_type *lval; /* Left  value. */
    value_type *rval; /* Right value. */
    register_  *dest; /* Result register. */

    explicit slt_expr(value_type *__lval,
                      value_type *__rval,
                      register_  *__dest)
        noexcept : lval(__lval), rval(__rval), dest(__dest) {}

    std::string data() const override {
        std::string buf = "slt";
        if(dynamic_cast <immediate *> (rval)) buf += 'i';
        return string_join(buf,' ', dest->data(),", ", lval->data(),", ", rval->data());
    }

    void accept(ASMvisitorbase *__v) override { __v->visitSltExpr(this); }

    ~slt_expr() override = default;
};


/* Equal expressions for booleans only. */
struct eq_expr : node {
    value_type *rval;
    register_  *dest;

    explicit eq_expr(value_type *__rval, register_ *__dest)
        noexcept : rval(__rval), dest(__dest) {}
    
    std::string data() const override {
        return string_join("sltu ", dest->data()," zero, ", rval->data());
    }

    void accept(ASMvisitorbase *__v) override { __v->visitEqExpr(this); }

    ~eq_expr() override = default;
};


/**
 * @brief Not expression for booleans only. 
 * 
 */
struct not_expr : node {
    value_type *rval;
    register_  *dest;

    explicit not_expr(value_type *__rval, register_ *__dest)
        noexcept : rval(__rval), dest(__dest) {}

    std::string data() const override {
        return string_join("seqz ", dest->data(),", ", rval->data());
    }

    void accept(ASMvisitorbase *__v) override { __v->visitNotExpr(this); }

    ~not_expr() override = default;
};


/* Special expr for converting boolean branches.  */
struct bool_expr   : node {
    value_type *cond; /* Condition. */
    block      *dest; /* Destination block. */

    explicit bool_expr(value_type *__cond, block *__dest)
        : cond(__cond), dest(__dest) {}

    std::string data() const override {
        return string_join("bnez ", cond->data(),", ",dest->label());
    }

    void accept(ASMvisitorbase *__v) override { __v->visitBoolExpr(this); }

    ~bool_expr() override = default;
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

    void accept(ASMvisitorbase *__v) override { __v->visitLoadMemory(this); }

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

    register_     *from;  /* Source register.  */
    address_type  *addr;  /* Address register. */
    register_     *temp;  /* Temporary register. */

    explicit store_memory(decltype (op) __op,
                          register_    *__src,
                          address_type *__addr)
        : op(__op), from(__src), addr(__addr),temp(nullptr) {}

    std::string data() const override {
        std::string __suffix;
        if(temp) __suffix = ", " + temp->data();
        return string_join(str[op],' ',from->data(),", ",addr->data(),__suffix);
    }

    void accept(ASMvisitorbase *__v) override { __v->visitStoreMemory(this); }

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

    void accept(ASMvisitorbase *__v) override { __v->visitLoadSymbol(this); }

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

    void accept(ASMvisitorbase *__v) override { __v->visitLoadImmediate(this); }

    ~load_immediate() override = default;
};


struct call_expr : node {
    function *func; /* Function name. */

    explicit call_expr(function *__func) : func(__func) {}

    std::string data() const override { return string_join("call ", func->name); }

    void accept(ASMvisitorbase *__v) override { __v->visitCallExpr(this); }

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

    void accept(ASMvisitorbase *__v) override { __v->visitMoveExpr(this); }

    ~move_expr() override = default;
};


struct jump_expr : node {
    block *label; /* Label name. */

    explicit jump_expr(block *__label) : label(__label) {}

    std::string data() const override { return string_join("j ", label->label()); }

    void accept(ASMvisitorbase *__v) override { __v->visitJumpExpr(this); }

    ~jump_expr() override = default;
};


struct return_expr : node {
    function *func; /* Function name. */

    explicit return_expr(function *__func) : func(__func) {}

    std::string data() const override {
        std::vector <std::string> buf;
        if(func->stk_size)
            buf.push_back(
                string_join(
                    "addi sp, sp, ", std::to_string(func->stk_size),"\n    "
                )
            );
        if(func->is_calling())
            buf.push_back("lw ra, -4(sp)\n    ");
        buf.push_back("ret");
        return string_join_array(buf.begin(), buf.end());
    }

    void accept(ASMvisitorbase *__v) override { __v->visitReturnExpr(this); }

    ~return_expr() override = default;
};


/* Address of a local variable. */
struct stack_address final : address_type {
    function    *func; /* Function . */
    IR::variable *var; /* Variable. */

    explicit stack_address(function *__func, IR::variable *__var)
        : func(__func), var(__var) {}

    std::string data() const override {
        return string_join(
            std::to_string(func->get_variable_offset(var)),
            "(sp)"
        );
    }

    ~stack_address() override = default;
};


/* Address of a global variable. */
struct global_address final : address_type {
    std::string name;
    explicit global_address(std::string_view __name) : name(__name) {}
    std::string data() const override { return name; }
    ~global_address() override = default;
};


/* Address of a given register. */
struct register_address final : address_type {
    register_ *reg;    /* Register. */
    immediate *offset; /* Offset. */

    explicit register_address(register_ *__reg, immediate *__offset)
        : reg(__reg), offset(__offset) {}

    std::string data() const override {
        return string_join(offset->data(), "(", reg->data(), ")");
    }

    ~register_address() override = default;
};


/* Address of temporaries. */
struct temporary_address final : address_type {
    function *func; /* Function. */
    size_t     pos; /* Position. */
    explicit temporary_address(function *__func, size_t __pos)
        : func(__func), pos(__pos) {}
    
    std::string data() const override {
        return string_join(
            std::to_string((func->arg_size() + pos) * 4),
            "(sp)"
        );
    }
};


struct global_information {
    std::vector <IR::initialization> rodata_list;
    std::vector <IR::initialization>   data_list;
    std::vector <ASM::function *>  function_list;
    std::map    <virtual_register *,size_t> reg_cnt;

    void print(std::ostream &os) const {
        os << "    .section .text\n";
        for(auto __p : function_list) {
            __p->init_function();
            os << __p->data() << '\n';
        }

        os << '\n';
        os << "    .section .data\n";
        for(auto [__var,__lit] : data_list) {
            os << "    .globl " << __var->name << '\n';
            os << __var->name << ":\n";
            auto __name = __lit->data();
            if(__name == "null") __name = "0";
            os << "    .word " << __lit->data() << '\n';
        }

        os << '\n';
        os << "    .section .rodata\n";
        for(auto [__var,__lit] : rodata_list) {
            os << "    .globl " << __var->name << '\n';
            os << __var->name << ":\n";
            os 
                << "    .asciz " 
                << safe_cast <IR::string_constant *> (__lit)->ASMdata() << "\n";
        }
    }
} ;





}
