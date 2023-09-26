#include "sccp.h"
#include "IRbasic.h"

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

    /* Special judge! */
    auto __zero = IR::create_boolean(0);
    auto __one1 = IR::create_boolean(1);

    if (__input_0 == __zero && __node->op == IR::compare_stmt::NE)
        return set_result(__input_1);
    if (__input_1 == __zero && __node->op == IR::compare_stmt::NE)
        return set_result(__input_0);
    if (__input_0 == __one1 && __node->op == IR::compare_stmt::EQ)
        return set_result(__input_1);
    if (__input_1 == __one1 && __node->op == IR::compare_stmt::EQ)
        return set_result(__input_0);

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

IR::string_constant *
    constant_calculator::try_get_string(IR::definition *__def) {
    static IR::string_constant __undef { "" };
    if (!__def) return nullptr;
    if (dynamic_cast <IR::undefined *> (__def)) return &__undef;
    if (auto __glo = dynamic_cast <IR::global_variable *> (__def)) {
        return dynamic_cast <IR::string_constant *> (__glo->const_val);
    } else  return nullptr;
}

void constant_calculator::visitCall(IR::call_stmt *__call) {
    auto &&__input = get_array();
    auto __func = __call->func;
    auto &&__builtin_pure = [&](std::string_view __name) -> IR::definition * {
        if (__name == "strcmp") {
            runtime_assert("strcmp() need 2 arguments!",__input.size() == 2);
            auto __input_0 = __input[0];
            auto __input_1 = __input[1];
            auto __lval = try_get_string(__input_0);
            auto __rval = try_get_string(__input_1);
            if (__lval && __rval) {
                return IR::create_integer(
                    std::strcmp(__lval->context.data(),
                                __rval->context.data()));
            } else return nullptr;
        }

        if (__name == "__String_ord__") {
            runtime_assert("__String_ord__() need 2 arguments!",__input.size() == 2);
            auto __input_0 = __input[0];
            auto __input_1 = __input[1];
            auto __lval = try_get_string(__input_0);
            auto __rval = dynamic_cast <IR::integer_constant *> (__input_1);
            if (__lval && __rval) {
                auto __index = __rval->value;
                if (__index < 0 || __index >= __lval->context.size())
                    return IR::create_undefined({},2);
                else return IR::create_integer(__lval->context[__index]);
            } else return nullptr;
        }

        if (__name == "strlen") {
            runtime_assert("strlen() need 1 argument!",__input.size() == 1);
            auto __input_0 = __input[0];
            auto __lval = try_get_string(__input_0);
            if (__lval) return IR::create_integer(__lval->context.size());
            else return nullptr;
        }

        if (__name == "__string_add__") {
            runtime_assert("__string_add__() need 2 arguments!",__input.size() == 2);
            auto __input_0 = __input[0];
            auto __input_1 = __input[1];
            auto __lval = try_get_string(__input_0);
            auto __rval = try_get_string(__input_1);
            if (__lval && __rval) {
                return IR::IRbasic::create_cstring(
                    __lval->context + __rval->context
                );
            } else return nullptr;
        }

        if (__name == "__String_parseInt__") {
            runtime_assert("__String_parseInt__() need 1 argument!",__input.size() == 1);
            auto __input_0 = __input[0];
            auto __lval = try_get_string(__input_0);
            if (__lval) {
                try {
                    return IR::create_integer(stoi(__lval->context));
                } catch(...) {
                    return IR::create_undefined({},2);
                }
            } else return nullptr;
        }

        if (__name == "__toString__") {
            runtime_assert("__toString__() need 1 argument!",__input.size() == 1);
            auto __input_0 = __input[0];
            auto __lval = dynamic_cast <IR::integer_constant *> (__input_0);
            if (__lval) {
                return IR::IRbasic::create_cstring(std::to_string(__lval->value));
            } else return nullptr;
        }
    
        if (__name == "__String_substring__") {
            runtime_assert("__String_substring__() need 3 arguments!",__input.size() == 3);
            auto __input_0 = __input[0];
            auto __input_1 = __input[1];
            auto __input_2 = __input[2];
            auto __lval = try_get_string(__input_0);
            auto __lpos = dynamic_cast <IR::integer_constant *> (__input_1);
            auto __rpos = dynamic_cast <IR::integer_constant *> (__input_2);
            if (__lval && __lpos && __rpos) {
                auto __l = __lpos->value;
                auto __r = __rpos->value;
                if (__l < 0 || __r < 0 || __l > __r || __r > __lval->context.size())
                    return IR::create_undefined({},3);
                else return IR::IRbasic::create_cstring(
                    __lval->context.substr(__l,__r - __l)
                );
            } else return nullptr;
        }
        return nullptr;
    };

    if (__func->is_builtin)
        return set_result(__builtin_pure(__func->name));
    else if (__func->is_pure_function()) {
        /* Only pure function with no dependency. */
        for(auto __arg : __call->args) {
            auto __lit = dynamic_cast <IR::literal *> (__arg);
            if (!__lit) return set_result(nullptr);
        }
        /* Tries to simulate the answer out! */
    }
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
    return __lhs == __rhs ? __lhs : nullptr;
}



}