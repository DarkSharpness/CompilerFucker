#pragma once
#include "ASMvalue.h"
#include <unordered_map>


namespace dark::ASM {

inline constexpr char __indent[8] = "    ";


struct dummy_expression final : std::vector <value_type> {
    enum : unsigned char {
        DEF = 0,
        USE = 1,
    } op;
    explicit dummy_expression (decltype(op) __op)
    noexcept : op(__op) {}
};


struct node : hidden_impl {
    virtual std::string data() const = 0;
    virtual ~node() = default;
    virtual void get_use(std::vector <Register *> &) const = 0;
    virtual Register *get_def() const = 0;
};

struct register_node : node {};
struct immediat_node : node {};
struct function_node : node {
    /* Just dummy use of some registers. */
    dummy_expression dummy_use {dummy_expression::USE};
    function *func; /* Function name. */
    function *self; /* Tail function. */
    Register *dest;
    enum : bool {
        CALL = 0,
        TAIL = 1,
    } op;

    void get_use(std::vector <Register *> & __vec)
    const override final {
        for (auto __p : dummy_use) __p.get_use(__vec);
    }

    Register *get_def() const override final { return dest; }

    explicit function_node(function *__func,decltype(op) __op)
    noexcept : func(__func), op(__op) {}
};

struct arith_base {
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
        [SRA]   = {'s','r','a'},
        [AND]   = {'a','n','d'},
        [OR_]   = {'o','r'},
        [XOR]   = {'x','o','r'},
    };
};


struct memory_base {
    using c_string = char [8];
    enum : unsigned char {
        BYTE = 0,
        WORD = 1,
    } op;
};


/* Register related. */
struct arith_register final : register_node , arith_base {
    Register *lval;
    Register *rval;
    Register *dest;

    explicit arith_register
        (decltype(op) __op, Register *__lval, Register *__rval, Register *__dest)
    noexcept : arith_base{__op}, lval(__lval), rval(__rval), dest(__dest) {}

    void get_use(std::vector <Register *> &__vec)
    const override { __vec.push_back(lval); __vec.push_back(rval); }
    Register *get_def() const override { return dest; }

    std::string data() const override {
        return string_join(__indent,
            str[op],' ',dest->data(),
            ", ", lval->data(),", ", rval->data()
        );
    }

    ~arith_register() override = default;
};


/* Register not related. */
struct arith_immediat final : immediat_node , arith_base {
    Register *lval;
    ssize_t   rval;
    Register *dest;

    explicit arith_immediat
        (decltype(op) __op, Register *__lval, ssize_t __rval, Register *__dest)
    noexcept : arith_base{__op}, lval(__lval), rval(__rval), dest(__dest) {}

    void get_use(std::vector <Register *> &__vec)
    const override { __vec.push_back(lval); }
    Register *get_def() const override { return dest; }

    std::string data() const override {
        return string_join(__indent,
            str[op],"i ",dest->data(),
            ", ", lval->data(),", ", std::to_string(rval)
        );
    }
    ~arith_immediat() override = default;
};


/* Sugar for addi. (Move value from one register to another.) */
struct move_register final : register_node {
    Register *from;
    Register *dest;

    explicit move_register(Register *__from, Register *__dest)
    noexcept : from(__from), dest(__dest) {}

    void get_use(std::vector <Register *> &__vec)
    const override { __vec.push_back(from); }
    Register *get_def() const override { return dest; }

    std::string data() const override {
        return string_join(__indent,
            "mv ", dest->data(), ", ", from->data()
        );
    }
    ~move_register() override = default;
};


/* Register related. */
struct slt_register final : register_node {
    Register *lval;
    Register *rval;
    Register *dest;

    explicit slt_register(Register *__lval, Register *__rval, Register *__dest)
    noexcept : lval(__lval), rval(__rval), dest(__dest) {}

    void get_use(std::vector <Register *> &__vec)
    const override { __vec.push_back(lval); __vec.push_back(rval); }
    Register *get_def() const override { return dest; }

    std::string data() const override {
        return string_join(__indent,
            "slt ", dest->data(), ", ", lval->data(), ", ", rval->data()
        );
    }

    ~slt_register() override = default;
};


/* Register related. */
struct slt_immediat final : immediat_node {
    Register *lval;
    ssize_t   rval;
    Register *dest;

    explicit slt_immediat(Register *__lval, ssize_t __rval, Register *__dest)
    noexcept : lval(__lval), rval(__rval), dest(__dest) {}

    void get_use(std::vector <Register *> &__vec)
    const override { __vec.push_back(lval); }

    Register *get_def() const override { return dest; }

    std::string data() const override {
        return string_join(__indent,
            "slti ", dest->data(), ", ", lval->data(), ", ", std::to_string(rval)
        );
    }

    ~slt_immediat() override = default;
};


/**
 * @brief Convert a value to bool.
 * val == 0 or val != 0
 */
struct bool_convert final : register_node {
    using c_string = char [8];
    enum : unsigned char {
        EQZ = 0,
        NEZ = 1,
    } op;
    inline static constexpr c_string str[2] = {
        [EQZ] = {'s','e','q','z'},
        [NEZ] = {'s','n','e','z'},
    };

    Register *from;
    Register *dest;

    explicit bool_convert(decltype(op) __op, Register *__from, Register *__dest)
    noexcept : op(__op), from(__from), dest(__dest) {}

    void get_use(std::vector <Register *> &__vec)
    const override { __vec.push_back(from); }
    Register *get_def() const override { return dest; }

    std::string data() const override {
        return string_join(__indent,
            str[op], ' ', dest->data(), ", ", from->data()
        );
    }

    ~bool_convert() override = default;
};


/**
 * @brief Convert a bool value to its not value.
 * Although it is not necessary, it can help
 * distinguish type of the register for optimizer.
 * 
 * In others words, it convert 1 to 0, and 0 to 1.
*/
struct bool_not final : register_node {
    Register *from;
    Register *dest;

    explicit bool_not(Register *__from, Register *__dest)
    noexcept : from(__from), dest(__dest) {}

    void get_use(std::vector <Register *> &__vec)
    const override { __vec.push_back(from); }
    Register *get_def() const override { return dest; }

    std::string data() const override {
        return string_join(__indent,
            "seqz ", dest->data(), ", ", from->data()
        );
    }

    ~bool_not() override = default;
};


/**
 * @brief Load a global variable address.
 * If used in load, it will be 
 * 
 */
struct load_symbol final : register_node {
    enum : unsigned char {
        HIGH = 0,
        LOW_ = 1,
        FULL = 2,
    } op;

    Register            *dest;
    IR::global_variable  *var;

    explicit load_symbol
        (decltype(op) __op,Register *__dest,IR::global_variable *__var)
    noexcept : op(__op), dest(__dest), var(__var) {}

    void get_use(std::vector <Register *> &) const override {}
    Register *get_def() const override { return dest; }

    std::string data() const override {
        if (op == HIGH) {
            return string_join(__indent,
                "lui ", dest->data(), ", %hi(", var->name, ")"
            );
        } else if (op == FULL) {
            return string_join(__indent,
                "la ", dest->data(), ", ", var->name
            );
        } else throw std::runtime_error("Not implemented!");
    }

    ~load_symbol() override = default;
};


/* Load from memory. */
struct load_memory final : register_node, memory_base {
    inline static constexpr c_string str[2] = {
        [BYTE] = {'l','b','u'},
        [WORD] = {'l','w'},
    };

    Register    *dest;
    value_type addr;

    explicit load_memory
        (decltype(op) __op, Register *__dest, value_type __addr)
    noexcept : memory_base {__op}, dest(__dest), addr(__addr) {}

    void get_use(std::vector <Register *> & __vec)
    const override { return addr.get_use(__vec); }
    Register *get_def() const override { return dest; }

    std::string data() const override {
        return string_join(__indent,
            str[op] , ' ', dest->data(), ", ", addr.data()
        );
    }

    ~load_memory() override = default;
};


/* Store from memory. */
struct store_memory final : register_node, memory_base {
    inline static constexpr c_string str[2] = {
        [BYTE] = {'s','b'},
        [WORD] = {'s','w'},
    };

    Register    *from;
    value_type   addr;

    explicit store_memory
        (decltype(op) __op, Register *__from, value_type __addr)
    noexcept : memory_base {__op}, from(__from), addr(__addr) {}

    void get_use(std::vector <Register *> & __vec)
    const override { __vec.push_back(from); addr.get_use(__vec); }
    Register *get_def() const override { return nullptr; }

    std::string data() const override {
        return string_join(__indent,
            str[op] , ' ', from->data(), ", ", addr.data()
        );
    }
    ~store_memory() override = default;
};


struct block : hidden_impl {
    inline static size_t label_count = 0;

    std::string name;
    std::vector <node *> expression;

    std::unordered_set <virtual_register *> use;
    std::unordered_set <virtual_register *> def;

    std::unordered_set <virtual_register *> livein;
    std::unordered_set <virtual_register *> liveout;

    std::vector <block *> prev;
    std::vector <block *> next;

    /* Default as 1 if not specially setted. */
    double loop_factor = 1;

    explicit block (std::string __name)
    noexcept : name {
        string_join(".L.",std::to_string(label_count++),'.',__name)
    } {}

    void emplace_back(node *__p) { expression.push_back(__p); }

    std::string data() const {
        std::vector <std::string> buf;
        buf.reserve(expression.size() << 1 | 1);
        buf.push_back(name + ":\n");
        for (auto __p : expression) {
            buf.push_back(__p->data());
            buf.push_back("\n");
        }
        return string_join_array(buf.begin(),buf.end());
    }

};


/**
 * @brief Stack frame.
 * 
 * ..........
 * Argument 9
 * Argument 8 (arg of this function)
 * 
 * -------------
 * ------------- (old sp)
 * 
 * Callee save register 1
 * 
 * -------------
 * 
 * Spilled register 1
 * 
 * -------------
 * 
 * local variable (from malloc -> alloc)
 * 
 * -------------
 * 
 * Argument 8 (arg of function called)
 * 
 * ------------- (new sp)
 * -------------
 * Temporary space
 * ..........
 * 
 * 
 */
struct function {
    std::string name;
    IR::function *func_ptr;

    ssize_t max_arg_size = -1; /* Maximum function arguments. */
    size_t  arg_offset   = 0;  /* Offset = max(max_arg_size - 8,0) << 2 */

    size_t var_count = 0; /* Count of alloca space.      */
    size_t stk_count = 0; /* Count of stack spill.       */

    /* Every function dummy defines all arguments. */
    dummy_expression dummy_def {dummy_expression::DEF};

    /* A map from variable to its address in stack. */
    std::unordered_map <IR::variable *,ssize_t>  var_map;
    std::unordered_map <size_t,virtual_register> vir_map;
    std::unordered_set <physical_register *> callee_save;

    /* Vector of all blocks. */
    std::vector <block *> blocks;

    bool save_ra = false; /* Whether there are non-tail function calls. */

    /* Emplace a new block to the back. */
    void emplace_back(block *__p) { blocks.push_back(__p); }

    /* Allocate a space. */
    ssize_t allocate(IR::local_variable *__var) {
        var_map[__var] = (var_count += __var->type.size());
        return var_count + arg_offset;
    }

    /* Tries to create a virtual register in the inner pool. */
    virtual_register *create_virtual() {
        const size_t __n = vir_map.size();
        return &vir_map.try_emplace(__n,__n).first->second;
    }

    /* Pass in one called function. */
    void update_size(size_t __n) noexcept
    { if (__n > max_arg_size) max_arg_size = __n; }

    /* Initialize argument offset after first scanning. */
    void init_arg_offset() noexcept
    { arg_offset = max_arg_size > 8 ? (max_arg_size - 8) << 2 : 0; }

    size_t   get_stack_space() const;
    void print(std::ostream &) const;
    std::string  return_data() const;
};


struct call_function final : function_node {
    explicit call_function(function *__func)
    noexcept : function_node {__func,CALL} {}
    std::string data() const override;
    ~call_function() override = default;
};


/* This is customized for builtin functions. */
struct call_builtin final : function_node {
    explicit call_builtin(function *__func)
    noexcept : function_node {__func,CALL} {}
    std::string data() const override;
    ~call_builtin() override = default;
};

struct ret_expression final : register_node {
    function *func;
    Register *rval;

    explicit ret_expression
        (function *__func,Register * __rval)
    noexcept : func(__func), rval(__rval) {}

    void get_use(std::vector <Register *> & __vec)
    const override { __vec.push_back(rval); }
    Register *get_def() const override { return nullptr; }

    /* Have much to be done! */
    std::string data() const override;
    ~ret_expression() override = default;
};


struct jump_expression final : node {
    block *dest;

    explicit jump_expression(block *__dest)
    noexcept : dest(__dest) {}

    void get_use(std::vector <Register *> &) const override {}
    Register *get_def() const override { return nullptr; }

    std::string data() const override
    { return string_join(__indent,"j ", dest->name); }

    ~jump_expression() override = default;
};


/* An IR-like branch. It should be reduced to others. */
struct branch_expression final : node {
    using c_string = char[8];
    enum : unsigned char {
        EQ = 0,
        NE = 1,
        GT = 2,
        GE = 3,
        LT = 4,
        LE = 5,
    } op;
    inline static constexpr c_string str[6] = {
        [EQ] = {'b','e','q'},
        [NE] = {'b','n','e'},
        [GT] = {'b','g','t'},
        [GE] = {'b','g','e'},
        [LT] = {'b','l','t'},
        [LE] = {'b','l','e'},
    };

    Register *lvar;
    Register *rvar;
    block *bran[2];

    /* Default compare with 0. */
    explicit branch_expression(Register *__cond, block *__b0, block *__b1)
    noexcept : op(NE), lvar{__cond}, rvar{get_physical(0)}
    { bran[0] = __b0; bran[1] = __b1; }

    /* Compare 2 branches. */
    explicit branch_expression
        (decltype(op) __op, Register *__lvar, Register *__rvar, block *__b0, block *__b1)
    noexcept : op(__op), lvar(__lvar), rvar(__rvar) { bran[0] = __b0; bran[1] = __b1; }

    void get_use(std::vector <Register *> & __vec) const override
    { __vec.push_back(lvar); __vec.push_back(rvar); }

    Register *get_def() const override { return nullptr; }

    std::string data() const override {
        return string_join(__indent,
            str[op], ' ', lvar->data(), ", ", rvar->data(), ", ", bran[0]->name,
            '\n', __indent, "j ", bran[1]->name
        );
    }

    ~branch_expression() override = default;
};


struct global_information {
    std::vector <IR::initialization> rodata_list;
    std::vector <IR::initialization>   data_list;
    std::vector <ASM::function *>  function_list;

    void print(std::ostream &os) const;
};


}