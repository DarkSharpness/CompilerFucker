#pragma once

#include "optmacro.h"
#include "IRnode.h"

#include <set>
#include <vector>

/* Optimizations. */
namespace dark::OPT {

enum side_effect_t : size_t {
    FX_STORE    = 0b0001,
    FX_CALL     = 0b0010,
    FX_NONE     = 0b0000, /* No effects. */
    FX_FULL     = 0b0011  /* Full effects. */
};

/* Join the effects of 2 effect type. */
inline side_effect_t operator | (side_effect_t __lhs, side_effect_t __rhs)
    noexcept {
    size_t __lval = static_cast <size_t> (__lhs);
    size_t __rval = static_cast <size_t> (__rhs);
    return static_cast <side_effect_t> (__lval | __rval);
}

/* Join the effects of 2 effect type. */
inline side_effect_t &operator |= (side_effect_t &__lhs, side_effect_t __rhs)
    noexcept {
    return __lhs = static_cast <side_effect_t> (__lhs | __rhs);
}

/* Intersect the effects of 2 effect type. */
inline side_effect_t operator & (side_effect_t __lhs, side_effect_t __rhs)
    noexcept {
    size_t __lval = static_cast <size_t> (__lhs);
    size_t __rval = static_cast <size_t> (__rhs);
    return static_cast <side_effect_t> (__lval & __rval);
}

/* Intersect the effects of 2 effect type. */
inline side_effect_t &operator &= (side_effect_t &__lhs, side_effect_t __rhs)
    noexcept {
    return __lhs = static_cast <side_effect_t> (__lhs & __rhs);
}

/* Return the side effects of a statement. */
inline side_effect_t get_side_fx(IR::node *__node) {
    if(dynamic_cast <IR::store_stmt *> (__node)) {
        return FX_NONE;
    } else if(dynamic_cast <IR::call_stmt *> (__node)) {
        return FX_CALL;
    } else {
        return FX_NONE;
    }
}

/* Return a certain effect from the statement. */
template <side_effect_t __fx>
inline side_effect_t remove_fx(side_effect_t __lhs) {
    return __lhs & static_cast <side_effect_t> (~__fx);
}


/**
 * @brief A node in the CFG.
 * 
 */
struct node {
    IR::block_stmt *block;

    std::vector <node *> prev;
    std::vector <node *> next;
    std::vector <node *> dom; /* Dominated dots. */
    std::vector <node *> fro; /* Dominated Frontier. */

    explicit node(IR::block_stmt *__block) : block(__block) {}

    const std::string &name() const noexcept { return block->label; }


    bool is_entry() const noexcept { return prev.empty(); }
    bool is_exit()  const noexcept { return next.empty(); }

    std::string data() const noexcept {
        std::vector <std::string> buf;
        buf.reserve(1 + next.size());
        buf.push_back(string_join("    Current block: ",name(),"    \n"));
        for(auto __p : next)
            buf.push_back(string_join("    Next block: ",__p->name(),"    \n"));
        return string_join_array(buf.begin(),buf.end());
    }
};

}