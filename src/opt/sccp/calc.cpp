#include "sccp.h"

namespace dark::OPT {

void constant_calculator::visitCompare(IR::compare_stmt *__node) {
    auto &&__input = get_array();
    runtime_assert("qwq?",__input.size() == 2);

    if(dynamic_cast <IR::undefined *> (__input[0]))
        return set_result(__input[0]);
    if(dynamic_cast <IR::undefined *> (__input[1]))
        return set_result(__input[1]);

    auto __lvar = dynamic_cast <IR::literal *> (__input[0]);
    auto __rvar = dynamic_cast <IR::literal *> (__input[1]);
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

    if (dynamic_cast <IR::undefined *> (__input[0]))
        return set_result(__input[0]);
    if (dynamic_cast <IR::undefined *> (__input[1]))
        return set_result(__input[1]);

    /* Undefined behavior: Divide by zero! */
    auto __zero = IR::create_integer(0);
    if (__input[1] == __zero) {
        switch(__node->op) {
            case IR::binary_stmt::SDIV:
            case IR::binary_stmt::SREM:
                return set_result(IR::create_undefined({},2));
            case IR::binary_stmt::MUL:
            case IR::binary_stmt::AND:
                return set_result(__zero);
        }
    }

    /* Special judge! */
    if (__input[0] == __zero) {
        switch(__node->op) {
            case IR::binary_stmt::SDIV:
            case IR::binary_stmt::SREM:
            case IR::binary_stmt::MUL:
            case IR::binary_stmt::AND:
                return set_result(__zero);
        }
    }


    auto __lvar = dynamic_cast <IR::literal *> (__input[0]);
    auto __rvar = dynamic_cast <IR::literal *> (__input[1]);
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
void constant_calculator::visitPhi(IR::phi_stmt *__node) {
    /**
     * TODO: Add a new function to judge whether a phi node is a constant.
     * 
    */
    return set_result(IR::create_undefined(__node->dest->type));
}

void constant_calculator::visitUnreachable(IR::unreachable_stmt *) {
    return set_result(nullptr);
}

}