#include "IRconst.h"


namespace dark::IR {

literal *const_folder::operator() (literal *__lit) const {
    if(auto __int = dynamic_cast <integer_constant *> (__lit))
        switch(op) {
            case ADD: return __int;
            case SUB: return create_integer(-__int->value);
            case REV: return create_integer(~__int->value);
            default : throw error("This should not happen!");
        }
    else if(auto __bool = dynamic_cast <boolean_constant *> (__lit))
        switch(op) {
            case NOT: return create_boolean(!__bool->value);
            default : throw error("This should not happen!");
        }
    else return nullptr;
}


literal *const_folder::operator()
    (integer_constant *__lhs,integer_constant *__rhs) const {
    if(!__lhs && !__rhs) return nullptr;
    if(__lhs && __rhs) {
        switch(op) {
            case ADD: return create_integer(__lhs->value +  __rhs->value);
            case SUB: return create_integer(__lhs->value -  __rhs->value);
            case MUL: return create_integer(__lhs->value *  __rhs->value);
            case DIV: return create_integer(__lhs->value /  __rhs->value);
            case REM: return create_integer(__lhs->value %  __rhs->value);
            case AND: return create_integer(__lhs->value &  __rhs->value);
            case OR : return create_integer(__lhs->value |  __rhs->value);
            case XOR: return create_integer(__lhs->value ^  __rhs->value);
            case SHL: return create_integer(__lhs->value << __rhs->value);
            case SHR: return create_integer(__lhs->value >> __rhs->value);
            case EQ : return create_boolean(__lhs->value == __rhs->value);
            case NE : return create_boolean(__lhs->value != __rhs->value);
            case LT : return create_boolean(__lhs->value <  __rhs->value);
            case GT : return create_boolean(__lhs->value >  __rhs->value);
            case LE : return create_boolean(__lhs->value <= __rhs->value);
            case GE : return create_boolean(__lhs->value >= __rhs->value);
            default : throw error("This should not happen!");
        }
    }

    switch(op) {
        case AND:
        case MUL:
            if (__rhs && __rhs->value == 0)
                return create_integer(0);
        case DIV:
        case REM:
            if (__lhs && __lhs->value == 0)
                return create_integer(0);
            break;
        case LE:
            if((__lhs && __lhs->value == INT32_MIN)
            || (__rhs && __rhs->value == INT32_MAX))
                return create_boolean(true);
            break;
        case GE:
            if((__lhs && __lhs->value == INT32_MAX)
            || (__rhs && __rhs->value == INT32_MIN))
                return create_boolean(true);
            break;
        case LT:
            if((__lhs && __lhs->value == INT32_MAX)
            || (__rhs && __rhs->value == INT32_MIN))
                return create_boolean(false);
            break;
        case GT:
            if((__lhs && __lhs->value == INT32_MIN)
            || (__rhs && __rhs->value == INT32_MAX))
                return create_boolean(false);
            break;
    }
    return nullptr;
}


literal *const_folder::operator()
    (boolean_constant *__lhs,boolean_constant *__rhs) const {
    if(!__lhs && !__rhs) return nullptr;
    if(__lhs && __rhs) {
        switch(op) {
            case AND: return create_boolean(__lhs->value && __rhs->value);
            case OR : return create_boolean(__lhs->value || __rhs->value);
            case EQ : return create_boolean(__lhs->value == __rhs->value);
            case NE : return create_boolean(__lhs->value != __rhs->value);
            default : throw error("This should not happen!");
        }
    }
    switch(op) {
        case AND: 
            if((__lhs && !__lhs->value) || (__rhs && !__rhs->value))
                return create_boolean(false);
            else break;
        case OR : 
            if((__lhs &&  __lhs->value) || (__rhs &&  __rhs->value))
                return create_boolean(true);
            else break;
    }
    return nullptr;
}

}