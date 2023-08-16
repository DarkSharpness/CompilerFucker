#pragma once

#include "IRnode.h"

namespace dark::IR {


/* A helper class that will attempt to fold constants if possible. */
struct const_folder {
    enum : unsigned char {
        ADD = 0,
        SUB = 1,
        MUL = 2,
        DIV = 3,
        REM = 4,
        SHL = 5,
        SHR = 6,
        AND = 7,
        OR  = 8,
        XOR = 9,
        NOT = 10,
        REV = 11,
        EQ  = 12,
        NE  = 13,
        LT  = 14,
        GT  = 15,
        LE  = 16,
        GE  = 17,
    } op;

    explicit const_folder(op_type __op) {
        if(!__op[1]) {
            switch(__op[0]) {
                case '+': op = ADD; break;
                case '-': op = SUB; break;
                case '*': op = MUL; break;
                case '/': op = DIV; break;
                case '%': op = REM; break;
                case '&': op = AND; break;
                case '|': op = OR;  break;
                case '^': op = XOR; break;
                case '~': op = REV; break;
                case '!': op = NOT; break;
                case '<': op = LT;  break;
                case '>': op = GT;  break;
                case '=': op = EQ;  break;
                default : throw error("This should not happen!");
            }
        } else if(__op[0] != __op[1]) {
            switch(__op[0]) {
                case '<': op = LE; break;
                case '>': op = GE; break;
                case '=': op = NE; break;
                default : throw error("This should not happen!");
            }
        } else {
            switch(__op[0]) {
                case '<': op = SHL; break;
                case '>': op = SHR; break;
                case '&': op = AND; break;
                case '|': op = OR;  break;
                default : throw error("This should not happen!");
            }
        }
    }

    explicit const_folder(decltype (binary_stmt::OR) __op) {
        op = static_cast <decltype (op)> (__op);
    }

    explicit const_folder(decltype (compare_stmt::EQ) __op) {
        switch(__op) {
            case compare_stmt::EQ: op = EQ; break;
            case compare_stmt::NE: op = NE; break;
            case compare_stmt::LT: op = LT; break;
            case compare_stmt::GT: op = GT; break;
            case compare_stmt::LE: op = LE; break;
            case compare_stmt::GE: op = GE; break;
        }
    }


    literal *operator() (literal *) const;
    literal *operator() (integer_constant *,integer_constant *) const;
    literal *operator() (boolean_constant *,boolean_constant *) const;

};





}