#pragma once
#include "ASMvalue.h"
#include <unordered_map>


namespace dark::ASM {

inline constexpr char __indent[8] = "    ";

struct node {
    virtual std::string data() const = 0;
    virtual ~node() = default;
    virtual void get_use(std::vector <Register *> &) const = 0;
    virtual Register *get_def() const = 0;
};

struct register_node : node {};
struct immediat_node : node {};

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
struct slt_immediat final : register_node {
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


/* Load one immediate. */
struct load_immediate final : immediat_node {
    Register *dest;
    ssize_t   rval;

    explicit load_immediate(Register *__dest, ssize_t __rval)
    noexcept : dest(__dest), rval(__rval) {}

    void get_use(std::vector <Register *> &) const override {}

    Register *get_def() const override { return dest; }

    std::string data() const override {
        return string_join(__indent,
            "addi ", dest->data(), ", zero, ", std::to_string(rval)
        );
    }
    ~load_immediate() override = default;
};


/*
    struct symbol_operation : register_node {
        using c_string = char [8];
        enum : unsigned char {
            LOW  = 0,
            HIGH = 1,
        } type;
        inline static constexpr c_string str[2] = {
            [LOW]  = {'%','l','o'},
            [HIGH] = {'%','h','i'},
        };

        Register    *dest;
        std::string symbol;

        explicit symbol_operation
            (decltype(type) __type, Register *__dest,std::string __str)
        noexcept : type(__type), dest(__dest), symbol(std::move(__str)) {}

        void get_use(std::vector <Register *> &) const override {}
        Register *get_def() const override { return dest; }

        ~symbol_operation() override = default;
    };
*/


struct load_symbol final : register_node {
    
};


/**
 * @brief Load high immediate of a global variable.
 * 
 */
struct load_high final : register_node {
    Register           *dest;
    IR::global_variable *var;

    explicit load_high (Register *__dest,IR::global_variable *__var)
    noexcept : dest(__dest), var(__var) {}

    void get_use(std::vector <Register *> &) const override {}
    Register *get_def() const override { return dest; }

    std::string data() const override {
        return string_join(__indent,
            "auipc ", dest->data(), ", %hi(", var->name, ")\n"
        );
    }

    ~load_high() override = default;
};


/* Load from memory. */
struct load_memory final : register_node {
    using c_string = char [8];
    enum : unsigned char {
        BYTE = 0,
        WORD = 1,
    } op;
    inline static constexpr c_string str[2] = {
        [BYTE] = {'l','b','u'},
        [WORD] = {'l','w'},
    };
    Register    *dest;
    address_type addr;

    explicit load_memory
        (decltype(op) __op, Register *__dest, address_type __addr)
    noexcept : op(__op), dest(__dest), addr(__addr) {}

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
struct store_memory final : register_node {
    using c_string = char [8];
    enum : unsigned char {
        BYTE = 0,
        WORD = 1,
    } op;
    inline static constexpr c_string str[2] = {
        [BYTE] = {'s','b'},
        [WORD] = {'s','w'},
    };
    Register    *from;
    address_type addr;

    explicit store_memory
        (decltype(op) __op, Register *__from, address_type __addr)
    noexcept : op(__op), from(__from), addr(__addr) {}

    void get_use(std::vector <Register *> & __vec)
    const override { return addr.get_use(__vec); }
    Register *get_def() const override { return nullptr; }

    std::string data() const override {
        return string_join(__indent,
            str[op] , ' ', from->data(), ", ", addr.data()
        );
    }
    ~store_memory() override = default;
};


struct block {
    inline static size_t label_count = 0;

    std::string name;
    explicit block (std::string __name)
    noexcept : name {
        string_join(".L.",std::to_string(label_count++),'.',__name)
    } {}

    std::vector <node *> expression;

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


struct function {
    std::string name;
    IR::function *func_ptr;

    ssize_t max_arg_size = -1; /* Maximum function arguments. */
    size_t  arg_offset   = 0;  /* Offset = max(max_arg_size - 8,0) << 2 */

    size_t vir_count = 0; /* Count of virtual registers. */
    size_t var_count = 0; /* Count of alloca space.      */
    size_t stk_count = 0; /* Count of stack spill.       */

    /* A map from variable to its address in stack. */
    std::unordered_map <IR::variable *,ssize_t> var_map;

    /* Vector of all blocks. */
    std::vector <block *> blocks;

    /* Emplace a new block to the back. */
    void emplace_back(block *__p) { blocks.push_back(__p); }

    /* Allocate a space. */
    size_t allocate(IR::variable *__var) {
        var_map[__var] = (var_count += __var->type.size());
        return var_count + arg_offset;
    }

    virtual_register *create_virtual() {
        return new virtual_register(vir_count++);
    }

};


struct call_function : register_node {
    function *func; /* Function name. */

    std::vector <value_type *> args; /* Arguments. */

    explicit call_function(function *__func) noexcept : func(__func) {}

    void get_use(std::vector <Register *> & __vec)
    const override {
        for (auto __p : args)
            if (auto __reg = dynamic_cast <Register *>(__p))
                __vec.push_back(__reg);
    }
    Register *get_def() const override { return nullptr; }

    std::string data() const override
    { return string_join(__indent,"call ", func->name); }

    ~call_function() override = default;
};


struct tail_function : register_node {
    function *func; /* Function name. */

    std::vector <value_type *> args; /* Arguments. */

    explicit tail_function(function *__func) noexcept : func(__func) {}

    void get_use(std::vector <Register *> & __vec)
    const override {
        for (auto __p : args)
            if (auto __reg = dynamic_cast <Register *>(__p))
                __vec.push_back(__reg);
    }
    Register *get_def() const override { return nullptr; }

    /* Have much to be done. */
    std::string data() const override;

    ~tail_function() override = default;

};


struct ret_expression : register_node {
    Register *from;

    explicit ret_expression(Register *__from) noexcept : from(__from) {}

    void get_use(std::vector <Register *> & __vec)
    const override { if (from) __vec.push_back(from); }
    Register *get_def() const override { return nullptr; }

    /* Have much to be done! */
    std::string data() const override;
    ~ret_expression() override = default;
};


struct jump_expression : node {
    block *dest = {};

    explicit jump_expression(block *__dest)
    noexcept : dest(__dest) {}

    void get_use(std::vector <Register *> &) const override {}
    Register *get_def() const override { return nullptr; }

    std::string data() const override
    { return string_join(__indent,"j ", dest->name); }

    ~jump_expression() override = default;
};


/* An IR-like branch. It should be reduced to others. */
struct branch_expression : node {
    Register *cond;
    block *bran[2] = {};

    explicit branch_expression(Register *__cond, block *__b0, block *__b1)
    noexcept : cond{__cond} { bran[0] = __b0; bran[1] = __b1; }

    void get_use(std::vector <Register *> & __vec)
    const override { if (cond) __vec.push_back(cond); }
    Register *get_def() const override { return nullptr; }

    std::string data() const override {
        return string_join(__indent,
            "bnez ", cond->data(), ", ", bran[0]->name,
            "\n", __indent, "j ", bran[1]->name
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