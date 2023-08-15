#pragma once

#include "utility.h"
#include "IRnode.h"

#include <string>

namespace dark::ASM {

struct block;
struct function;


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


struct value_type {
    virtual std::string data() const = 0;
    virtual ~value_type() = default;
};

/**
 * @brief Immediate value.
 * 
 * 
 */
struct immediate : value_type {
    int value;
    explicit immediate(int __v) : value(__v) {}
    std::string data() const override final { return std::to_string(value); }
    ~immediate() override = default;
};

/**
 * @brief Register value.
 * Virtual or physical registers.
 * 
 */
struct register_ : value_type {};

/* Symbol of a global variable. */
struct symbol    : value_type {
    std::string name;
    explicit symbol(std::string_view __name) : name(__name) {}
    std::string data() const override final { return name; }
    ~symbol() override = default;
};


/* Risc-V register. */
struct physical_register : register_ {
    register_type type;

    physical_register() = delete;

    inline static physical_register *get_register(register_type type) {
        unsigned char index = static_cast <unsigned char> (type);
        if (index >= 32) throw dark::error("Invalid register type");
        if (!reg[index]) reg[index] = new physical_register(type);
        return reg[index];
    }

    inline static struct static_deletor {
        ~static_deletor() { for (auto &__p : reg) delete __p; }
    } __deletor = {};

    std::string data() const override final {
        switch(static_cast <unsigned char> (type)) {
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

  private:
    physical_register(register_type __type) : type(__type) {}
    inline static physical_register *reg[32] = {};
};


/* A virtual register. */
struct virtual_register  : register_ {
    size_t index;

    explicit virtual_register(size_t __n) : index(__n) {}

    std::string data() const override final { return 'v' + std::to_string(index); }
    ~virtual_register() override = default;
};


/**
 * @brief Test whether the constant is a low immeadiate.
 * @param __v Variable to test.
 * @return Whether the constant is a low immeadiate.
 */
inline bool is_low_immediate(int __v) {
    return __v >= -2048 && __v <= 2047;
}




}