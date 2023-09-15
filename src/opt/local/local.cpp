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

    for(auto __stmt : __block->stmt) {
        if (auto __bin = dynamic_cast <IR::binary_stmt *> (__stmt))
        { update_bin(__bin); continue; }
        // if (auto __cmp = dynamic_cast <IR::compare_stmt *> (__stmt))
        // { update_cmp(__cmp); continue; }
        // if (auto __load = dynamic_cast <IR::load_stmt *> (__stmt))
        // { update_load(__load); continue; }
        // if (auto __store = dynamic_cast <IR::store_stmt *> (__stmt))
        // { update_store(__store); continue; }
        // if (auto __get = dynamic_cast <IR::get_stmt *> (__stmt))
        // { update_get(__get); continue; }
        // if (auto __br = dynamic_cast <IR::branch_stmt *> (__stmt))
        // { update_br(__br); continue; }
    }
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
    return std::addressof(__cache[__count++] =
        {.new_def = __def,.def_node = nullptr,.neg_flag = false}
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
 * (X *  C1) / C2   --> X * (C1 / C2)        // iff C2 divides C1
 * (X *  C1) % C2   --> 0 + 0 = 0            // iff C2 divides C1
 * (X << C1) % C2   --> 0 + 0 = 0            // iff C2 divides pow(2,C1)
 *  Shift:
 * (X << C1) << C2  --> X << (C1 + C2)
 * (X >> C1) >> C2  --> X >> (C1 + C2)
 * (X << C1) >> C2  --> X << (C1 - C2) or X >> (C2 - C1)
 * 
 * Special case in merging operators.
 * (C1 - X) + C2    --> (C1 + C2) - X
 * C1 - (X + C2)    --> (C1 - C2) - X
 * C1 - (C2 - X)    --> (C1 - C2) + X
 * 
 * Maybe I will write this:
 * (0 - X) / X      --> 0 + -1 = -1
 * (0 - X) % X      --> 0 + 0 = 0
 * X / (0 - X)      --> 0 + -1 = -1
 * X % (0 - X)      --> 0 + 0 = 0
 * 
 * Special case for non-constant test:
 * (X ^ Y) ^ X      --> Y + 0 = Y
 * (X | Y) & X      --> X + 0 = X
 * (X & Y) | X      --> X + 0 = X
 * (X | Y) | X      --> X | Y
 * (X & Y) & X      --> X & Y
 * 
 * (X - Y) + Y      --> X + 0 = X
 * (X + Y) - X      --> Y + 0 = Y
 * (X * Y) / X      --> Y + 0 = Y
 * (X * Y) % X      --> 0 + 0 = 0
 * 
 * Negative generation rule:
 * 0 - X        --> (-X)
 * X * (-1)     --> (-X)
 * X / (-1)     --> (-X)
 *
 * @return Whether the statement is a single definition.
*/
bool local_optimizer::update_bin(IR::binary_stmt *__stmt) {
    usage_info __pool[2]; /* Cache pool. */

    /* Replace a negative with its positive value. */
    auto &&__update_neg = [&](usage_info *__old,bool __index)
        -> usage_info * {
        auto *__tmp = dynamic_cast <IR::temporary *> (__old->new_def);
        auto __iter = use_info.find(__tmp);
        if (__iter == use_info.end()) return __old;
        return std::addressof(__pool[__index] = __iter->second);
    };

    auto *__lhs = get_use_info(__stmt->lvar);
    auto *__rhs = get_use_info(__stmt->rvar);

    /* Try to update the usage info. */
    auto &&__reload = [&](usage_info *__ptr) -> void {
        if ( __ptr->new_def) __ptr = get_use_info(__ptr->new_def);
        if (!__ptr->new_def) __ptr->new_def = __ptr->def_node->get_def();
        if (!__ptr->new_def) throw;
    };

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

    auto &&__work_neg_l = [&]() -> void {
        if (__stmt->op == IR::binary_stmt::ADD) {
            __stmt->op = IR::binary_stmt::SUB;
            std::swap(__lhs,__rhs);
            std::swap(__stmt->lvar,__stmt->rvar);
            __rhs = __update_neg(__rhs,0);
            return;
        }
        auto *__lit = dynamic_cast <IR::integer_constant *> (__rhs->new_def);
        /* (-X) / C = X / (-C) */
        /* (-X) * C = X * (-C) */
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
                /* Natural fall-through. */
            /* X % (-Y) == X % Y */
            case IR::binary_stmt::SREM: __rhs = __update_neg(__rhs,0); return;
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
        /* This is used to help reduce computation in next const calc. */
        __stmt->op   = IR::binary_stmt::ADD;
        __stmt->lvar = __tmp;
        __stmt->rvar = IR::create_integer(0);
        use_info[__stmt->dest] = *get_use_info(__tmp);
        return true;
    };

    /* Merge operators to try generate deadcode. */
    auto &&__merge_operator =
        [__stmt](IR::integer_constant *__rvar,IR::node *__node)-> bool {
        const auto *__temp = dynamic_cast <IR::binary_stmt *> (__node);
        if (!__temp) return false;
        int __lval = 0; 
        int __rval = __rvar->value;
        auto  __op = __temp->op;
        auto __def = __temp->lvar;
        if (auto * __lvar = dynamic_cast <IR::integer_constant *> (__temp->rvar)) {
            __lval = __lvar->value;
        } else { /* Right var is not a constant case! */
            /* Special case: for optimization of MUL,DIV and REM,
                (0 - X) can be treated as (X * (-1))  */
            if (__lvar = dynamic_cast <IR::integer_constant *> (__def)) {
                auto __lval = __lvar->value;
                /* (C1 - X) + C2    --> (C1 + C2) - X  */
                if (__op == IR::binary_stmt::ADD) {
                    __stmt->op   = IR::binary_stmt::SUB;
                    __stmt->rvar = __temp->rvar;
                    __stmt->lvar = IR::create_integer(
                        __lval + __rval
                    ); return true;
                } else if (__lval == 0 &&
                    (__op == IR::binary_stmt::SREM ||
                     __op == IR::binary_stmt::SDIV ||
                     __op == IR::binary_stmt::MUL)) {
                    __def = __temp->rvar;
                    __op  = IR::binary_stmt::MUL;
                } 
            } else return false;
        }

        switch(__stmt->op) {
            /* (X op C1) op C2 = X op (C1 op C2)    (where op := AND,OR,XOR) */
            case IR::binary_stmt::AND:
            case IR::binary_stmt::OR:
            case IR::binary_stmt::XOR:
                if (__op == __stmt->op) {
                    __stmt->lvar = __def;
                    __stmt->rvar = IR::create_integer(
                        __op == IR::binary_stmt::AND ?
                            __lval & __rvar->value :
                        __op == IR::binary_stmt::XOR ?
                            __lval ^ __rvar->value :
                            __lval | __rvar->value
                    ); return true;
                } return false;

            /* (X op C1) + C2 = X + (C2 op C1)    (where op := ADD,SUB) */
            case IR::binary_stmt::ADD:
                /* Hacking method! */
                if (__op <= IR::binary_stmt::SUB) {
                    __stmt->lvar = __def;
                    __stmt->rvar = IR::create_integer(
                        __op == IR::binary_stmt::ADD ?
                            __rvar->value + __lval :
                            __rvar->value - __lval
                    ); return true;
                } return false;

            /* (X << C1) << C2 = X << (C1 + C2) */
            case IR::binary_stmt::SHL:
                if (__op == IR::binary_stmt::SHL) {
                    __stmt->lvar = __def;
                    __stmt->rvar = IR::create_integer(
                        __lval + __rvar->value
                    ); return true;
                } return false;

            /* (X >> C1) >> C2  --> X >> (C1 + C2) */
            case IR::binary_stmt::ASHR:
                if (__op == IR::binary_stmt::ASHR) {
                    __stmt->lvar = __def;
                    __stmt->rvar = IR::create_integer(
                        __lval + __rvar->value
                    ); return true;
                } return false;

            /* (0 - X) == X * (-1)                */
            /* (X * C1) * C2   --> X * (C1 * C2)  */
            /* (X << C1) * C2  --> X * (C2 << C1) */
            case IR::binary_stmt::MUL:
                if(__op == IR::binary_stmt::MUL) {
                    __stmt->lvar = __def;
                    __stmt->rvar = IR::create_integer(
                        __lval * __rvar->value
                    ); return true;
                } else if (__op == IR::binary_stmt::SHL) {
                    __stmt->lvar = __def;
                    __stmt->rvar = IR::create_integer(
                        __rvar->value << __lval
                    ); return true;
                } return false;

            /* (X * C1) / C2   --> X * (C1 / C2)        // iff C2 divides C1        */
            case IR::binary_stmt::SDIV:
                if (__op == IR::binary_stmt::MUL) {
                    if(__lval % __rvar->value == 0) {
                        __stmt->lvar = __def;
                        auto __tmp = __lval / __rvar->value;
                        /* Strength reduction. */
                        if (is_pow_of_2(__tmp)) {
                            __stmt->op = IR::binary_stmt::SHL;
                            __stmt->rvar = IR::create_integer(
                                __builtin_ctz(__tmp)
                            );
                        } else {
                            __stmt->rvar = IR::create_integer(__tmp);
                        } return true;
                    }
                } return false;

            /* (X * C1) % C2   --> 0 + 0 = 0            // iff C2 divides C1        */
            /* (X << C1) % C2  --> 0 + 0 = 0            // iff C2 divides pow(1,C2) */
            case IR::binary_stmt::SREM:
                if (__op == IR::binary_stmt::MUL) {
                    if(__lval % __rvar->value == 0) {
                        __stmt->lvar = __stmt->rvar = IR::create_integer(0);
                        __stmt->op   = IR::binary_stmt::ADD;
                    } return true;
                } else if(__op == IR::binary_stmt::SHL) {
                    if((1 << __lval) % __rvar->value == 0) {
                        __stmt->lvar = __stmt->rvar = IR::create_integer(0);
                        __stmt->op   = IR::binary_stmt::ADD;
                    } return true;
                } return false;

            default: runtime_assert("How can this ......"); return false;
        }
    };


    /**
     * ---------------------------
     * Main body of the function.
     * ---------------------------
    */


    /* Swap the constant to the right if possible. */
    if (auto __lit = dynamic_cast <IR::integer_constant *> (__lhs->new_def)) {
        switch(__stmt->op) {
            /* Symmetric case: swap to right hand side. */
            case IR::binary_stmt::ADD:
            case IR::binary_stmt::MUL:
            case IR::binary_stmt::AND:
            case IR::binary_stmt::OR:
            case IR::binary_stmt::XOR:
                std::swap(__lhs,__rhs);
                std::swap(__stmt->lvar,__stmt->rvar);
        }
    }

    if (__lhs->neg_flag && __rhs->neg_flag) {
        __work_neg_lr(); __reload(__lhs); __reload(__rhs);
    } else if (__lhs->neg_flag) {
        __work_neg_l();  __reload(__lhs);
    } else if (__rhs->neg_flag) {
        __work_neg_r();  __reload(__rhs);
    }

    runtime_assert("No negative now!",!__lhs->neg_flag,!__rhs->neg_flag);

    /* Try constant calculation first. */
    if (__try_const(__lhs->new_def,__rhs->new_def)) return true;

    /* Strength reduction and replacement. */
    IR::integer_constant *__lit;
    if (__lit = dynamic_cast <IR::integer_constant *> (__rhs->new_def)) {
        switch(__stmt->op) {
            case IR::binary_stmt::SUB:
                __stmt->op = IR::binary_stmt::ADD;
                __rhs->new_def = __lit = IR::create_integer(-__lit->value);
                break;
            case IR::binary_stmt::MUL:
                if (is_pow_of_2(__lit->value)) {
                    __stmt->op = IR::binary_stmt::SHL;
                    __rhs->new_def = __lit = IR::create_integer(
                        __builtin_ctz(__lit->value)
                    );
                }
        }
    } else if (__lhs->new_def == __rhs->new_def
            && __stmt->op     == __stmt->ADD) {
        __stmt->op = IR::binary_stmt::SHL;
        __rhs->new_def = __lit = IR::create_integer(1);
    }

    __stmt->lvar = __lhs->new_def;
    __stmt->rvar = __rhs->new_def;

    /* The binary is updated. So visit down recursively. */
    if (__lit && __merge_operator(__lit,__lhs->def_node)) {
        /* Try constant calculation again. */
        if (__try_const(__stmt->lvar,__stmt->rvar)) return true;
        else return update_bin(__stmt);
    }

    /**
     * Special case in merging operators.
     * (C1 - X) + C2    --> (C1 + C2) - X
     * C1 - (X + C2)    --> (C1 - C2) - X
     * C1 - (C2 - X)    --> (C1 - C2) + X
     * 
     * (0 - X) / X      --> 0 + -1 = -1
     * (0 - X) % X      --> 0 + 0 = 0
     * X / (0 - X)      --> 0 + -1 = -1
     * X % (0 - X)      --> 0 + 0 = 0
     * 
     * Special case for non-constant test.
     * 
     *  Symmetric case:
     * (X ^ Y) ^ X      --> Y + 0 = Y
     * (X | Y) & X      --> X + 0 = X
     * (X & Y) | X      --> X + 0 = X
     * (X | Y) | X      --> X | Y
     * (X & Y) & X      --> X & Y
     * (X - Y) + Y      --> X + 0 = X
     * 
     *  Non-symmetric case:
     * (X + Y) - X      --> Y + 0 = Y
     * (X - Y) - X      --> 0 - Y = (-Y)
     * X - (X - Y)      --> Y + 0 = Y
     * X - (X + Y)      --> 0 - Y = (-Y)
     * (X * Y) / X      --> Y + 0 = Y
     * (X * Y) % X      --> 0 + 0 = 0
     * 
     * Negative generation rule:
     * 0 - X    --> (-X)
     * X * (-1) --> (-X)
     * X / (-1) --> (-X)
    */
    if ([__stmt](IR::integer_constant * __lit,IR::node *__node) -> bool {
        if (!__lit || __stmt->op != IR::binary_stmt::SUB) return false;
        const auto *__temp = dynamic_cast <IR::binary_stmt *> (__node);
        if (!__temp) return false;

        /* C1 - (X + C2)    --> (C1 - C2) - X   */
        if (auto __tmp = dynamic_cast <IR::integer_constant *> (__temp->rvar);
            __tmp != nullptr && __temp->op == IR::binary_stmt::ADD) {
            __stmt->op   = IR::binary_stmt::SUB;
            __stmt->lvar = IR::create_integer(
                __lit->value - __tmp->value
            ); return true;
        }

        /* C1 - (C2 - X)    --> (C1 - C2) + X   */
        if (auto __tmp = dynamic_cast <IR::integer_constant *> (__temp->lvar);
            __tmp != nullptr && __temp->op == IR::binary_stmt::SUB) {
            __stmt->op = IR::binary_stmt::ADD;
            __stmt->lvar = IR::create_integer(
                __lit->value - __tmp->value
            ); return true;
        }
        return false;
    } (dynamic_cast <IR::integer_constant *> (__lhs->new_def),__rhs->def_node)) {
        /* Try constant calculation again. */
        if (__try_const(__stmt->lvar,__stmt->rvar)) return true;
        else return update_bin(__stmt);
    }

    /* Special case for non-constant test. */
    auto &&__merge_symmetric = [__stmt]
        (IR::binary_stmt *__temp,IR::temporary *__def,uint32_t __state) -> bool {
        /* (X - Y) + Y  --> X + 0 = X */
        if (__stmt->op == IR::binary_stmt::ADD) {
            if (__temp->op == IR::binary_stmt::SUB && (__state & 0b10)) {
                __stmt->op = IR::binary_stmt::SUB;
                __stmt->lvar = __temp->rvar;
                __stmt->rvar = IR::create_integer(0);
                return true;
            } return false;
        }

        switch(__stmt->op) {
            /* (X & Y) | X  --> X + 0 = X   */
            /* (X | Y) | X  --> X | Y       */
            case IR::binary_stmt::AND:
            case IR::binary_stmt::OR:
                /* Hacking method! 15 = 7 + 8 = AND + OR */
                if (__temp->op + __stmt->op == 15) {
                    __stmt->op   = IR::binary_stmt::ADD;
                    __stmt->lvar = __def;
                    __stmt->rvar = IR::create_integer(0);
                    return true;
                } else if (__temp->op == __stmt->op) {
                    __stmt->lvar = __temp->lvar;
                    __stmt->rvar = __temp->rvar;
                    return true;
                } return false;

            case IR::binary_stmt::XOR:
                if (__temp->op == IR::binary_stmt::XOR) {
                    __stmt->op   = IR::binary_stmt::ADD;
                    __stmt->lvar = __state & 0b1 ? __temp->rvar : __temp->lvar;
                    __stmt->rvar = IR::create_integer(0);
                    return true;
                } return false;
        }
        return false;
    };

    auto &&__merge_lhs = [&,__stmt]() -> bool {
        auto *__temp = dynamic_cast <IR::binary_stmt *> (__lhs->def_node);
        auto *__def  = dynamic_cast <IR::temporary *>   (__rhs->new_def);
        if (!__temp || !__def) return false;
        uint32_t __state = (__temp->lvar == __def) | ((__temp->rvar == __def) << 1);
        if (!__state) return false;
        if (__merge_symmetric(__temp,__def,__state)) return true;

        /* LHS case. */
        if (__stmt->op == IR::binary_stmt::SUB) {
            if (__temp->op == IR::binary_stmt::ADD) {
                __stmt->op   = IR::binary_stmt::ADD;
                __stmt->lvar = __state & 0b1 ? __temp->rvar : __temp->lvar;
                __stmt->rvar = IR::create_integer(0);
                return true;
            } else if (__temp->op == IR::binary_stmt::SUB && (__state & 0b1)) {
                __stmt->op   = IR::binary_stmt::SUB;
                __stmt->lvar = IR::create_integer(0);
                __stmt->rvar = __temp->rvar;
                return true;
            }
        } if (__temp->op == IR::binary_stmt::MUL) {
            if (__stmt->op == IR::binary_stmt::SDIV) {
                __stmt->op   = IR::binary_stmt::ADD;
                __stmt->lvar = __state & 0b1 ? __temp->rvar : __temp->lvar;
                __stmt->rvar = IR::create_integer(0);
                return true;
            } else if (__stmt->op == IR::binary_stmt::SREM) {
                __stmt->op   = IR::binary_stmt::ADD;
                __stmt->lvar = __stmt->rvar = IR::create_integer(0);
                return true;
            }
        }
        return false;
    };

    auto &&__merge_rhs = [&,__stmt]() -> bool {
        auto *__temp = dynamic_cast <IR::binary_stmt *> (__rhs->def_node);
        auto *__def  = dynamic_cast <IR::temporary *>   (__lhs->new_def);
        if (!__temp || !__def) return false;
        uint32_t __state = (__temp->lvar == __def) | ((__temp->rvar == __def) << 1);
        if (!__state) return false;
        if (__merge_symmetric(__temp,__def,__state)) return true;

        /* LHS case. */
        if (__stmt->op == IR::binary_stmt::SUB) {
            if (__temp->op == IR::binary_stmt::ADD) {
                __stmt->op   = IR::binary_stmt::SUB;
                __stmt->lvar = IR::create_integer(0);
                __stmt->rvar = __state & 0b1 ? __temp->rvar : __temp->lvar;
                return true;
            } else if (__temp->op == IR::binary_stmt::SUB && (__state & 0b1)) {
                __stmt->op   = IR::binary_stmt::ADD;
                __stmt->lvar = __temp->rvar;
                __stmt->rvar = IR::create_integer(0);
                return true;
            }
        }
        return false;
    };

    if (__merge_lhs() || __merge_rhs()) {
        /* Try constant calculation again. */
        if (__try_const(__stmt->lvar,__stmt->rvar)) return true;
        else return update_bin(__stmt);
    }

    /* Generate negative information. */
    auto &&__generate_neg = [__stmt](usage_info &__info) -> bool {
        __info.neg_flag = true;
        __info.new_def  = __stmt->rvar;
        __info.def_node = __stmt;
        return false;
    };

    auto &__info = use_info[__stmt->dest];

    /* Swap const to right if possible. */
    if (__lit = dynamic_cast <IR::integer_constant *> (__stmt->lvar)) {
        switch(__stmt->op) {
            /* Symmetric case: swap to right hand side. */
            case IR::binary_stmt::ADD:
            case IR::binary_stmt::MUL:
            case IR::binary_stmt::AND:
            case IR::binary_stmt::OR:
            case IR::binary_stmt::XOR:
                std::swap(__stmt->lvar,__stmt->rvar);
        }
    }

    /* Generate negative information. */
    if (__lit = dynamic_cast <IR::integer_constant *> (__stmt->rvar)) {
        if (__lit->value == 0 && __stmt->op == __stmt->SUB)
            return __generate_neg(__info);
        else if (__lit->value == -1 &&
        (__stmt->op == __stmt->MUL || __stmt->op == __stmt->SDIV)) {
            __stmt->op  = __stmt->SUB;
            __stmt->rvar = __stmt->lvar;
            __stmt->lvar = IR::create_integer(0);
            return __generate_neg(__info);
        }
    }

    __info.neg_flag = false;
    __info.new_def  = __stmt->dest;
    __info.def_node = __stmt;

    return false;
}


/**
 * Strength reduction:
 * X != 1   --> (X == 0)        // iff boolean
 * X == 1   --> (X != 0) = X    // iff boolean
 * X <= Y   --> (Y >= X)
 * X >  Y   --> (Y <  X)
 * 
 * General case:
 * (X != Y) == 0    --> (X == Y)
 * (X == Y) == 0    --> (X != Y)
 * (X <  Y) == 0    --> (X >= Y)
 * (X >= Y) == 0    --> (X <  Y)
 * 
 * Booleans only:
 * (X != Y) != Y    --> (X != 0) = X
 * (X == Y) == Y    --> (X != 0) = X
 * (X == Y) != Y    --> (X == 0)
 * (X != Y) == Y    --> (X == 0)
 * 
 * 
 * Branch opt: (asm)
 * %tmp = icmp ...
 * br %tmp ..
 * --> b... (asm)
 * 
 * 
*/

}