#pragma once
#include "IRnode.h"
#include <unordered_set>

namespace dark::ASM {


struct Register {
    virtual std::string data() const = 0;
    virtual ~Register() = default;
};


struct physical_register : Register {
    enum {
        zero    = 0, /* Hardwired zero */
        ra      = 1, /* Return address */
        sp      = 2, /* Stack pointer */
        gp      = 3, /* Global pointer (It is now wasted. As middle temporary) */
        tp      = 4, /* Thread pointer */
        t0      = 5, /* Temporaries */
        t1      = 6, /* Temporaries */
        t2      = 7, /* Temporaries */
        s0      = 8, /* Saved registers */
        fp      = 8, /* Saved registers */
        s1      = 9, /* Saved registers */
        a0      = 10, /* Function arguments/return values */
        a1      = 11, /* Function arguments/return values */
        a2      = 12, /* Function arguments */
        a3      = 13, /* Function arguments */
        a4      = 14, /* Function arguments */
        a5      = 15, /* Function arguments */
        a6      = 16, /* Function arguments */
        a7      = 17, /* Function arguments */
        s2      = 18, /* Saved registers */
        s3      = 19, /* Saved registers */
        s4      = 20, /* Saved registers */
        s5      = 21, /* Saved registers */
        s6      = 22, /* Saved registers */
        s7      = 23, /* Saved registers */
        s8      = 24, /* Saved registers */
        s9      = 25, /* Saved registers */
        s10     = 26, /* Saved registers */
        s11     = 27, /* Saved registers */
        t3      = 28, /* Temporaries */
        t4      = 29, /* Temporaries */
        t5      = 30, /* Temporaries */
        t6      = 31, /* Temporaries */
    };
    /* Index of real register. */
    size_t index;
    explicit physical_register(size_t __index) : index(__index) {}
    std::string data() const override {
        switch(index) {
            case 0: return "zero";
            case 1: return "ra";
            case 2: return "sp";
            case 3: return "gp";
            case 4: return "tp";
            case 5: return "t0";
            case 6: return "t1";
            case 7: return "t2";
            case 8: return "s0";
            case 9: return "s1";
            case 10: return "a0";
            case 11: return "a1";
            case 12: return "a2";
            case 13: return "a3";
            case 14: return "a4";
            case 15: return "a5";
            case 16: return "a6";
            case 17: return "a7";
            case 18: return "s2";
            case 19: return "s3";
            case 20: return "s4";
            case 21: return "s5";
            case 22: return "s6";
            case 23: return "s7";
            case 24: return "s8";
            case 25: return "s9";
            case 26: return "s10";
            case 27: return "s11";
            case 28: return "t3";
            case 29: return "t4";
            case 30: return "t5";
            case 31: return "t6";
            default: throw dark::error("Invalid register type");
        }
    }
    ~physical_register() override = default;
};


struct virtual_register : Register {
    size_t index;
    explicit virtual_register(size_t __index) : index(__index) {}
    std::string data() const override {
        size_t __len = std::__detail::__to_chars_len(index);
        std::string __str(__len + 1, '\0');
        __str[0] = 'v';
        std::__detail::__to_chars_10_impl(&__str[1], __len, index);
        return __str;
    }
    ~virtual_register() override = default;
};



/* Create a physical register. */
inline physical_register *get_physical(size_t __n) {
    runtime_assert("Register index out of range.", __n < 32);
    static physical_register __reg[32] = {
        physical_register {0},
        physical_register {1},
        physical_register {2},
        physical_register {3},
        physical_register {4},
        physical_register {5},
        physical_register {6},
        physical_register {7},
        physical_register {8},
        physical_register {9},
        physical_register {10},
        physical_register {11},
        physical_register {12},
        physical_register {13},
        physical_register {14},
        physical_register {15},
        physical_register {16},
        physical_register {17},
        physical_register {18},
        physical_register {19},
        physical_register {20},
        physical_register {21},
        physical_register {22},
        physical_register {23},
        physical_register {24},
        physical_register {25},
        physical_register {26},
        physical_register {27},
        physical_register {28},
        physical_register {29},
        physical_register {30},
        physical_register {31}
    };
    return __reg + __n;
}

struct function;


struct global_address {
    Register *reg; /* Register with high part of address. */
    IR::global_variable *var; /* Global variable. */
    std::string data() const {
        if (reg == nullptr) return var->name;
        return string_join("%lo(",var->name,"), ",reg->data());
    }
};


struct stack_address {
    function *func;
     /* index < 0 function argument (excess) || index > 0 temporary. */
    ssize_t  index;
    std::string data() const;
};


struct pointer_address {
    Register * reg;
    ssize_t offset;
    std::string data() const {
        return string_join(
            std::to_string(offset),'(',reg->data(),')'
        );
    }

    pointer_address operator += (ssize_t __offset) {
        offset += __offset; return *this;
    }
};


/**
 * @brief It has 2 special cases.
 * Global address: It is the immediate value of the global variable.
 * It can be used in load/store with respect to the register.
 * 
 * 
 */
struct value_type {
    union {
        global_address  global;
        pointer_address pointer;
    };
    enum : bool {
        GLOBAL, /* Global variable.                */
        POINTER, /* In pointer with given bias.     */
    } type;

    explicit value_type(nullptr_t) {}

    value_type(global_address __global)
    noexcept : global(__global), type(GLOBAL) {}

    value_type(pointer_address __pointer)
    noexcept : pointer(__pointer), type(POINTER) {}

    std::string data() const {
        switch(type) {
            case GLOBAL:  return global.data();
            case POINTER: return pointer.data();
        } throw error("?");
    }

    void get_use(std::vector <Register *> &__use) const {
        static_assert( /* This is necessary ! */
            offsetof(global_address,reg) == offsetof(pointer_address,reg)
        );
        __use.push_back(pointer.reg);
    }
};


}




