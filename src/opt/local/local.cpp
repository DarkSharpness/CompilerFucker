#include "peephole.h"


namespace dark::OPT {


inline bool is_pow_of_2(int __val) noexcept
{ return __val && __val == (1 << __builtin_ctz(__val)); }


local_optimizer::local_optimizer(IR::function *__func,node *) {
    for(auto __block : __func->stmt) optimize(__block);
}


void local_optimizer::optimize(IR::block_stmt *__block) {
    use_info.clear();
    mem_info.clear();
    remove_set.clear();

    // for(auto __stmt : __block->stmt) {
    //     if (auto __bin = dynamic_cast <IR::binary_stmt *> (__stmt))
    //     { update_bin(__bin); continue; }
    //     if (auto __cmp = dynamic_cast <IR::compare_stmt *> (__stmt))
    //     { update_cmp(__cmp); continue; }
    //     if (auto __load = dynamic_cast <IR::load_stmt *> (__stmt))
    //     { update_load(__load); continue; }
    //     if (auto __store = dynamic_cast <IR::store_stmt *> (__stmt))
    //     { update_store(__store); continue; }
    //     if (auto __get = dynamic_cast <IR::get_stmt *> (__stmt))
    //     { update_get(__get); continue; }
    //     if (auto __br = dynamic_cast <IR::branch_stmt *> (__stmt))
    //     { update_br(__br); continue; }
    // }
}


local_optimizer::usage_info *
    local_optimizer::get_use_info(IR::definition *__def) {
    static constexpr size_t SZ = 4; /* Cache size! */
    static usage_info  __cache[SZ];
    static size_t      __count = 0;
    if (__count == SZ) __count = 0;

    if (auto *__tmp = dynamic_cast <IR::temporary *> (__def)) {
        auto __iter = use_info.find(__tmp);
        if (__iter != use_info.end())
            return std::addressof(__cache[__count++] = __iter->second);
    }
    return std::addressof(
        __cache[__count++] = {.new_def = __def,.def_node = nullptr}
    );
}

/**
 * Swap operands if the left one is a constant
 * for those symmetric operators: + * & | ^ 
 * 
 * Strength reduction and replacement:
 * X - C        --> X + (-C)
 * X + X        --> X << 1
 * X * pow(2,n) --> X << n
 * 
 * 
 * Negative elimination rule:
 * 0 - Y        --> (-Y)
 * X + (-Y)     --> X - Y
 * (-Y) + X     --> X - Y
 * X - (-Y)     --> X + Y
 * (-X) * C     --> X * (-C)    // iff non-power-of-2 C
 * (-X) * (-Y)  --> X * Y
 * (-X) / C     --> X / (-C)
 * C / (-X)     --> (-C) / X
 * (-X) / (-Y)  --> X / Y
 * X % (-Y)     --> X % Y
 * 
 * 
 * Merge operators to try generate deadcode (to be removed~):
 *  Bitwise:
 * (X & C1) & C2    --> X & (C1 & C2)
 * (X | C1) | C2    --> X | (C1 | C2)
 * (X ^ C1) ^ C2    --> X ^ (C1 ^ C2)
 *  Add or Sub:
 * (X + C1) + C2    --> X + (C1 + C2)
 * (X - C1) + C2    --> X + (C2 - C1)
 *  Mult or Div:
 * (X << C1) * C2   --> X * (C2 << C1)
 * (X *  C1) * C2   --> X * (C1 * C2)
 * (X << C1) / C2   --> X << (C1 - log2(C2)) // iff C2 divides pow(2,C1)
 * (X *  C1) / C2   --> X * (C1 / C2)        // iff C2 divides C1
 * (X << C1) % C2   --> 0 + 0 = 0            // iff C2 divides pow(2,C1)
 * (X *  C1) % C2   --> 0 + 0 = 0            // iff C2 divides C1
 *  Shift:
 * (X << C1) << C2  --> X << (C1 + C2)
 * (X >> C1) >> C2  --> X >> (C1 + C2)
 * (X << C1) >> C2  --> X << (C1 - C2) or X >> (C2 - C1)
 * 
 * Special case in merging.
 * (C1 - X) + C2    --> (C1 + C2) - X
 * C1 - (X + C2)    --> (C1 - C2) - X
 * C1 - (C2 - X)    --> (C1 - C2) + X
 * X * (-1)         --> 0 - X
 * X / (-1)         --> 0 - X
 * 
 * 
 * Special case for non-constant test.
 * (X ^ Y) ^ X      --> Y + 0 = Y
 * (X | Y) & X      --> X + 0 = X
 * (X & Y) | X      --> X + 0 = X
 * (X - Y) + Y      --> X + 0 = X
 * (X + Y) - X      --> Y + 0 = Y
 * (X * Y) / X      --> Y + 0 = Y
 * 
 * 
*/
void local_optimizer::update_bin(IR::binary_stmt *__stmt) {


    usage_info __cache[2]; /* Cache pool. */

    auto &&__update_neg = [&](usage_info *__old,bool __index)
        -> usage_info * {
        auto *__tmp = dynamic_cast <IR::temporary *> (__old->new_def);
        auto __iter = use_info.find(__tmp);
        if (__iter == use_info.end()) return __old;
        return std::addressof(__cache[__index] = __iter->second);
    };

    auto *__lhs = get_use_info(__stmt->lvar);
    auto *__rhs = get_use_info(__stmt->rvar);
    IR::integer_constant *__lit;

    auto &&__work_neg_lr = [&]() -> void {
        switch(__stmt->op) {
            case IR::binary_stmt::MUL:
            case IR::binary_stmt::SDIV:
                __lhs = __update_neg(__lhs,0);
                __rhs = __update_neg(__rhs,1);
                break;
            case IR::binary_stmt::SREM:
                __lhs->new_def = nullptr;
                __rhs = __update_neg(__rhs,1);
                break;
            default:
                __lhs->new_def = nullptr;
                __rhs->new_def = nullptr;
        }
    };

    auto &&__reload = [&](usage_info *__ptr) -> void {
        if ( __ptr->new_def) __ptr = get_use_info(__ptr->new_def);
        if (!__ptr->new_def) __ptr->new_def = __ptr->def_node->get_def();
    };

    auto &&__work_neg_l = [&]() -> void {
        if (__stmt->op == IR::binary_stmt::ADD) {
            __stmt->op = IR::binary_stmt::SUB;
            std::swap(__lhs,__rhs);
            __rhs = __update_neg(__rhs,0);
            return;
        }
        __lit = dynamic_cast <IR::integer_constant *> (__rhs->new_def);
        if (__lit && (
            __stmt->op == IR::binary_stmt::SDIV || 
           (__stmt->op == IR::binary_stmt::MUL  && !is_pow_of_2(__lit->value))
        ))
            __rhs->new_def = IR::create_integer(-__lit->value);
        else
            __lhs->new_def = nullptr;
    
    };

    auto &&__work_neg_r = [&]() -> void {
        IR::integer_constant *__lit;
        switch(__stmt->op) {
            case IR::binary_stmt::ADD:
            case IR::binary_stmt::SUB:
                /* Hacking method! */
                __stmt->op = static_cast <decltype(__stmt->op)> (1 - __stmt->op);
                __rhs = __update_neg(__rhs,0);
                return;
            /* X % (-Y) == X % Y */
            case IR::binary_stmt::SREM: return;
            /* C / (-Y) == (-C) / Y */
            case IR::binary_stmt::SDIV:
                if (__lit = dynamic_cast <IR::integer_constant *> (__lhs->new_def))
                { __lhs->new_def = IR::create_integer(-__lit->value); return; }
        }
        __rhs->new_def = nullptr;
    };

    auto &&__try_const = [&](IR::definition *__l,IR::definition *__r) -> bool {
        this->__cache = {__l,__r};
        calc.set_array(this->__cache);
        calc.visitBinary(__stmt);
        auto __tmp = static_cast <IR::definition *> (calc);
        if (!__tmp) return false;
        use_info[__stmt->dest] = *get_use_info(__tmp);
        return true;
    };

    /* Merge operators to try generate deadcode. */
    auto &&__merge_operator = [&](IR::integer_constant *__rvar) -> void {
        const auto *__temp = dynamic_cast <IR::binary_stmt *> (__lhs->def_node);
        if (!__temp) return;
        auto *const __lvar = dynamic_cast <IR::integer_constant *> (__temp->rvar);
        if (!__lvar) return;
        switch(__stmt->op) {
            /* (X op C1) op C2 = X op (C1 op C2)    (where op := AND,OR,XOR) */
            case IR::binary_stmt::AND:
            case IR::binary_stmt::OR:
            case IR::binary_stmt::XOR:
                if (__temp->op == __stmt->op) {
                    __stmt->lvar = __temp->lvar;
                    __stmt->rvar = IR::create_integer(
                        __temp->op == IR::binary_stmt::AND ?
                            __lvar->value & __rvar->value :
                        __temp->op == IR::binary_stmt::XOR  ?
                            __lvar->value ^ __rvar->value :
                            __lvar->value | __rvar->value
                    );
                } return;

            /* (X op C1) + C2 = X + (C1 op C2)    (where op := ADD,SUB) */
            case IR::binary_stmt::ADD:
                /* Hacking method! */
                if (__temp->op <= IR::binary_stmt::SUB) {
                    __stmt->lvar = __temp->lvar;
                    __stmt->rvar = IR::create_integer(
                        __temp->op == IR::binary_stmt::ADD ?
                            __lvar->value + __rvar->value :
                            __rvar->value - __lvar->value 
                    );
                } return;

            /* (X << C1) << C2 = X << (C1 + C2) */
            case IR::binary_stmt::SHL:
                if (__temp->op == IR::binary_stmt::SHL) {
                    __stmt->lvar = __temp->lvar;
                    __stmt->rvar = IR::create_integer(
                        __lvar->value + __rvar->value
                    );
                } return;

            /* (X >> C1) >> C2  --> X >> (C1 + C2)                   */
            case IR::binary_stmt::ASHR:
                if (__temp->op == IR::binary_stmt::ASHR) {
                    __stmt->lvar = __temp->lvar;
                    __stmt->rvar = IR::create_integer(
                        __lvar->value + __rvar->value
                    );
                } return;

            /* (X *  C1) * C2   --> X * (C1 * C2) */
            /* (X << C1) * C2   --> X * (C2 << C1) */
            case IR::binary_stmt::MUL:
                if(__temp->op == IR::binary_stmt::MUL) {
                    __stmt->lvar = __temp->lvar;
                    __stmt->rvar = IR::create_integer(
                        __lvar->value * __rvar->value
                    );
                } else if (__temp->op == IR::binary_stmt::SHL) {
                    __stmt->lvar = __temp->lvar;
                    __stmt->rvar = IR::create_integer(
                        __rvar->value << __lvar->value
                    );
                } return;

            /* (X *  C1) / C2   --> X * (C1 / C2)        // iff C2 divides C1        */
            /* (X << C1) / C2   --> X << (C1 - log2(C2)) // iff C2 divides pow(2,C1) */
            case IR::binary_stmt::SDIV:
                if (__temp->op == IR::binary_stmt::MUL) {
                    if(__lvar->value % __rvar->value == 0 && __rvar->value > 0) {
                        __stmt->lvar = __temp->lvar;
                        __stmt->rvar = IR::create_integer(
                            __lvar->value / __rvar->value
                        );
                    }
                } else if (__temp->op == IR::binary_stmt::SHL) {
                    if ((1 << __lvar->value) % __rvar->value == 0 && __rvar->value > 0) {
                        __stmt->lvar = __temp->lvar;
                        __stmt->rvar = IR::create_integer(
                            __lvar->value - __builtin_ctz(__rvar->value)
                        );
                    }
                } return;

            /* (X *  C1) % C2   --> 0 + 0 = 0            // iff C2 divides C1        */
            /* (X << C1) % C2   --> 0 + 0 = 0            // iff C2 divides pow(2,C1) */
            case IR::binary_stmt::SREM:
                if (__temp->op == IR::binary_stmt::MUL) {
                    if(__lvar->value % __rvar->value == 0 && __rvar->value > 0) {
                        __stmt->lvar = __stmt->rvar = IR::create_integer(0);
                        __stmt->op   = IR::binary_stmt::ADD;
                    }
                } else if (__temp->op == IR::binary_stmt::SHL) {
                    if ((1 << __lvar->value) % __rvar->value == 0 && __rvar->value > 0) {
                        __stmt->lvar = __stmt->rvar = IR::create_integer(0);
                        __stmt->op   = IR::binary_stmt::ADD;
                    }
                } return;

        }



    };

    /* Main body of the function. */

    /* Swap the constant to the right if possible. */
    if (__lit = dynamic_cast <IR::integer_constant *> (__lhs->new_def)) {
        switch(__stmt->op) {
            /* Symmetric case: swap to right hand side. */
            case IR::binary_stmt::ADD:
            case IR::binary_stmt::MUL:
            case IR::binary_stmt::AND:
            case IR::binary_stmt::OR:
            case IR::binary_stmt::XOR:
                std::swap(__stmt->lvar,__stmt->rvar);
                std::swap(__lhs,__rhs);
        }
    }

    if (__lhs->neg_flag && __rhs->neg_flag) {
        __work_neg_lr(); __reload(__lhs); __reload(__rhs);
    } else if (__lhs->neg_flag) {
        __work_neg_l(); __reload(__lhs);
    } else if (__rhs->neg_flag) {
        __work_neg_r(); __reload(__rhs);
    }

    runtime_assert("No negative now!",!__lhs->neg_flag,!__rhs->neg_flag);

    /* Try constant calculation first. */
    if (__try_const(__lhs->new_def,__rhs->new_def)) return;

    /* Strength reduction and replacement. */
    if (__lit = dynamic_cast <IR::integer_constant *> (__rhs->new_def)) {
        switch(__stmt->op) {
            case IR::binary_stmt::SUB:
                __stmt->op = IR::binary_stmt::ADD;
                __lit = IR::create_integer(-__lit->value);
            case IR::binary_stmt::MUL:
                if (is_pow_of_2(__lit->value)) {
                    __stmt->op = IR::binary_stmt::SHL;
                    __rhs->new_def = IR::create_integer(
                        __builtin_ctz(__lit->value)
                    );
                }
        }
        __merge_operator(__lit);
    } else if (__lhs->new_def == __rhs->new_def
            && __stmt->op     == __stmt->ADD) {
        __stmt->op = IR::binary_stmt::SHL;
        __rhs->new_def = __lit = IR::create_integer(1);
        __merge_operator(__lit);
    }


}

}