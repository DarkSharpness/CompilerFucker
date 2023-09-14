#include "sccp.h"

namespace dark::OPT {

void constant_calculator::visitCompare(IR::compare_stmt *__node) {
    auto &&__input = get_array();
    runtime_assert("qwq?",__input.size() == 2);

    auto __input_0 = __input[0];
    auto __input_1 = __input[1];

    if(dynamic_cast <IR::undefined *> (__input_0))
        return set_result(__input_0);
    if(dynamic_cast <IR::undefined *> (__input_1))
        return set_result(__input_1);

    if (__input_0 == __input_1 && __input_0) {
        switch(__node->op) {
            case IR::compare_stmt::EQ:
            case IR::compare_stmt::LE:
            case IR::compare_stmt::GE:
                return set_result(IR::create_boolean(true));
            case IR::compare_stmt::NE:
            case IR::compare_stmt::LT:
            case IR::compare_stmt::GT:
                return set_result(IR::create_boolean(false));
        }
    }

    auto __lvar = dynamic_cast <IR::literal *> (__input_0);
    auto __rvar = dynamic_cast <IR::literal *> (__input_1);
    if(!__lvar || !__rvar) return set_result(nullptr);

    bool __bool; /* Result holder. */
    auto __lval = __lvar->get_constant_value();
    auto __rval = __rvar->get_constant_value();
    switch(__node->op) {
        case IR::compare_stmt::EQ : __bool = (__lval == __rval); break;
        case IR::compare_stmt::NE : __bool = (__lval != __rval); break;
        case IR::compare_stmt::LT : __bool = (__lval <  __rval); break;
        case IR::compare_stmt::LE : __bool = (__lval <= __rval); break;
        case IR::compare_stmt::GT : __bool = (__lval >  __rval); break;
        case IR::compare_stmt::GE : __bool = (__lval >= __rval); break;
        default: runtime_assert("Unknown compare operator!");
    }

    return set_result(IR::create_boolean(__bool));
}

void constant_calculator::visitBinary(IR::binary_stmt *__node) {
    auto &&__input = get_array();
    runtime_assert("qwq??",__input.size() == 2);
    auto __input_0 = __input[0];
    auto __input_1 = __input[1];

    if (dynamic_cast <IR::undefined *> (__input_0))
        return set_result(__input_0);
    if (dynamic_cast <IR::undefined *> (__input_1))
        return set_result(__input_1);

    /* Undefined behavior: Divide by zero! */
    auto __zero = IR::create_integer(0);
    auto __one1 = IR::create_integer(1);
    if (__input_1 == __zero) {
        switch(__node->op) {
            case IR::binary_stmt::SDIV: /* x / 0 = undef */
            case IR::binary_stmt::SREM: /* x % 0 = undef */
                return set_result(IR::create_undefined({},2));

            case IR::binary_stmt::MUL: /* x * 0 = 0 */
            case IR::binary_stmt::AND: /* x & 0 = 0 */
                return set_result(__zero);

            case IR::binary_stmt::OR:  /* x | 0 = x */
            case IR::binary_stmt::XOR: /* x ^ 0 = x */
            case IR::binary_stmt::SHL: /* x << 0 = x */
            case IR::binary_stmt::ASHR:/* x >> 0 = x */
            case IR::binary_stmt::ADD: /* x + 0 = x */
            case IR::binary_stmt::SUB: /* x - 0 = x */
                return set_result(__input_0);
        }
    }

    /* Special judge! */
    if (__input_0 == __zero) {
        switch(__node->op) {
            case IR::binary_stmt::SDIV: /* 0 / x = 0 */
            case IR::binary_stmt::SREM: /* 0 % x = 0 */
            case IR::binary_stmt::MUL:  /* 0 * x = 0 */
            case IR::binary_stmt::AND:  /* 0 & x = 0 */
            case IR::binary_stmt::SHL:  /* 0 << x = 0 */
            case IR::binary_stmt::ASHR: /* 0 >> x = 0 */
                return set_result(__zero);

            case IR::binary_stmt::OR:   /* 0 | x = x */
            case IR::binary_stmt::XOR:  /* 0 ^ x = x */
            case IR::binary_stmt::ADD:  /* 0 + x = x */
                return set_result(__input_1);
        }
    }

    /* Special judge! */  /* 1 * x = x */
    if (__input_0 == __one1 && __node->op == IR::binary_stmt::MUL)
        return set_result(__input_1);

    if (__input_1 == __one1) {
        switch(__node->op) {
            case IR::binary_stmt::MUL:  /* x * 1 = x */
            case IR::binary_stmt::SDIV: /* x / 1 = x */
                return set_result(__input_0);
            case IR::binary_stmt::SREM: /* x % 1 = 0 */
                return set_result(__zero);
        }
    }

    /* Dangerous! */
    if (__input_0 == __input_1 && __input_0) {
        switch(__node->op) {
            case IR::binary_stmt::AND: /* x & x = x */
            case IR::binary_stmt::OR:  /* x | x = x */
                return set_result(__input_0);

            case IR::binary_stmt::SUB:  /* x - x = 0 */
            case IR::binary_stmt::SREM: /* x % x = 0 */
            case IR::binary_stmt::ASHR: /* x >> x = 0 */
            case IR::binary_stmt::XOR:  /* x ^ x = 0 */
                return set_result(__zero);

            case IR::binary_stmt::SDIV: /* x / x = 1 */
                return set_result(__one1);
        }
    }

    auto __lvar = dynamic_cast <IR::literal *> (__input_0);
    auto __rvar = dynamic_cast <IR::literal *> (__input_1);
    if(!__lvar || !__rvar) return set_result(nullptr);

    auto __lval = __lvar->get_constant_value();
    auto __rval = __rvar->get_constant_value();
    switch(__node->op) {
        case IR::binary_stmt::ADD : __lval += __rval;  break;
        case IR::binary_stmt::SUB : __lval -= __rval;  break;
        case IR::binary_stmt::MUL : __lval *= __rval;  break;
        case IR::binary_stmt::SDIV: __lval /= __rval;  break;
        case IR::binary_stmt::SREM: __lval %= __rval;  break;
        case IR::binary_stmt::AND : __lval &= __rval;  break;
        case IR::binary_stmt::OR  : __lval |= __rval;  break;
        case IR::binary_stmt::XOR : __lval ^= __rval;  break;
        case IR::binary_stmt::SHL : __lval <<= __rval; break;
        case IR::binary_stmt::ASHR: __lval >>= __rval; break;
        default: runtime_assert("Unknown binary operator!");
    }

    return set_result(IR::create_integer(__lval));
}

void constant_calculator::visitJump(IR::jump_stmt *) {
    return set_result(nullptr);
}

void constant_calculator::visitBranch(IR::branch_stmt *) {
    return set_result(nullptr);
}

void constant_calculator::visitCall(IR::call_stmt *) {
    return set_result(nullptr);
}

void constant_calculator::visitLoad(IR::load_stmt *) {
    return set_result(nullptr);
}

void constant_calculator::visitStore(IR::store_stmt *) {
    return set_result(nullptr);
}

void constant_calculator::visitReturn(IR::return_stmt *) {
    return set_result(nullptr); 
}

void constant_calculator::visitAlloc(IR::allocate_stmt *) {
    return set_result(nullptr);
}

void constant_calculator::visitGet(IR::get_stmt *) {
    return set_result(nullptr);
}

/* Special judge! */
void constant_calculator::visitPhi(IR::phi_stmt *__phi) {
    auto &&__input = get_array();
    IR::temporary  *__def = __phi->dest;
    IR::definition *__tmp = nullptr;

    for(auto __use : __input) {
        if (__use == nullptr) return set_result(nullptr);
        if (__use == __def) continue;
        /**
         * @brief Simple rule as below:
         * Self = undef.
         * undef + undef = undef.
         * undef + def   = single phi def.
         * def   + def   = non-single phi.
        */
        if(dynamic_cast <IR::undefined *> (__use)) continue;
        else if(__tmp == nullptr) __tmp = __use;
        /* Multiple definition: not a zero/ single phi! */
        else if(__tmp != __use) return set_result(nullptr);
    }

    return set_result(__tmp ? __tmp : IR::create_undefined(__def->type));
}

void constant_calculator::visitUnreachable(IR::unreachable_stmt *) {
    return set_result(nullptr);
}


/**
 * @brief Merge the definition of 2 IR value. \n
 * undef + def_0 ==> def_0 \n
 * def_0 + def_0 ==> def_0 \n
 * def_0 + def_1 ==> null (non-const)
*/
IR::definition *merge_definition(IR::definition *__lhs,IR::definition *__rhs) {
    if (dynamic_cast <IR::undefined *> (__lhs)) return __rhs;
    if (dynamic_cast <IR::undefined *> (__rhs)) return __lhs;
    if (__lhs == __rhs) return __lhs;
    else                return nullptr;
}



}