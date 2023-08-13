#pragma once

#include "utility.h"

#include <string>

namespace dark::ASM {

enum class register_type : unsigned char {
    zero    = 0, /* Hardwired zero */
    ra      = 1, /* Return address */
    sp      = 2, /* Stack pointer */
    gp      = 3, /* Global pointer */
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

/* Risc-V register */
struct physical_register {
    register_type type;

    physical_register() = delete;

    inline static physical_register *get_register(register_type type) {
        unsigned char index = static_cast <unsigned char> (type);
        if (index >= 32) throw dark::error("Invalid register type");
        if (reg[index] == nullptr) reg[index] = new physical_register(type);
        return reg[index];
    }

    inline static
    struct static_deletor {
        ~static_deletor() { for (auto &__p : reg) delete __p; }
    } __deletor = {};

  private:
    physical_register(register_type __type) : type(__type) {}
    inline static physical_register *reg[32] = {};
};



}