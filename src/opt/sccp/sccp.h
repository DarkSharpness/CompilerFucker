#pragma once

#include "optnode.h"
#include <queue>
#include <unordered_set>
#include <unordered_map>


namespace dark::OPT {

struct constant_propagatior;
struct constant_calculator : IR::IRvisitorbase {
  protected:
    void *data; /* This argument is responsible for passing the params. */
    using array_type = std::vector<IR::definition *>;
    explicit constant_calculator() = default;

    const array_type &get_array()
    { return *static_cast <const array_type *> (data); }

    explicit operator IR::definition *()
    { return static_cast <IR::definition *> (data); }

    void set_result(IR::definition *__def) 
    { data = static_cast <void *> (__def); }

    friend constant_propagatior;

  public:
 
    /* Work out the const folding result. */
    [[nodiscard]] inline IR::definition *
        work(IR::node *__ctx,const std::vector <IR::definition *> &__vec) {
        data = static_cast <void *> (const_cast <array_type *> (&__vec));
        visit(__ctx);
        return static_cast <IR::definition *> (*this);
    }

    void visitBlock(IR::block_stmt*) override {}
    void visitFunction(IR::function*) override {}
    void visitInit(IR::initialization*) override {}

    void visitCompare(IR::compare_stmt *) override;
    void visitBinary(IR::binary_stmt *) override;
    void visitJump(IR::jump_stmt *) override;
    void visitBranch(IR::branch_stmt *) override;
    void visitCall(IR::call_stmt *) override;
    void visitLoad(IR::load_stmt *) override;
    void visitStore(IR::store_stmt *) override;
    void visitReturn(IR::return_stmt *) override;
    void visitAlloc(IR::allocate_stmt *) override;
    void visitGet(IR::get_stmt *) override;
    void visitPhi(IR::phi_stmt *) override;
    void visitUnreachable(IR::unreachable_stmt *) override;
    ~constant_calculator() override = default;
};


struct constant_propagatior {
    std::vector <IR::definition *> __cache;
    constant_calculator calc;

    /* We assume that all globals are unknown now. */
    struct constant_info {
        /* If null, non-constant. Otherwise,undefined / constant. */
        std::map <IR::temporary *,IR::definition *> value_map;

        using iterator_type = decltype(value_map)::iterator;

        /* Return the definition mapped from current definition. */
        IR::definition *get_literal(IR::definition *__old) const {
            /* No mapping for undefined / literal type. */
            if (dynamic_cast <IR::literal *>   (__old)) return __old;
            if (dynamic_cast <IR::undefined *> (__old)) return __old;

            /* All temporaries must be in the map.  */
            if (auto __tmp = dynamic_cast <IR::temporary *> (__old))
                return value_map.at(__tmp);

            /* Other cases are non-constant. */
            return nullptr;
        }

        bool update_at(iterator_type __pos,IR::definition *__new) {
            runtime_assert("Fail!",__pos != value_map.end());
            auto &__old = __pos->second;
            /* First, handle with the cases of undefined. */
            if (dynamic_cast <IR::undefined *> (__new)) return false;
            if (dynamic_cast <IR::undefined *> (__old))
            { __old = __new; return true; }

            auto __lval = dynamic_cast <IR::literal *> (__new);
            auto __rval = dynamic_cast <IR::literal *> (__old);
            /**
             * This is a magic equation!
             * If both literal constant, then compare their value (ptr).
             * If both are not literal, of course they are equal(null == null).
             * Otherwise, they are not equal.
            */
            bool __ret  = __lval != __rval;
            /* Not equal : update as nullptr. */
            if (__ret) __old = nullptr;
            return __ret;
        }

        /* Tries to update inner constant state. */
        bool update(IR::temporary *__tmp,IR::definition *__new) 
        { return update_at(value_map.find(__tmp),__new); }


    };

    struct usage_info {
        std::vector <IR::node *> use_list;
        IR::node                *def_node;
    };

    std::unordered_map <IR::block_stmt *,constant_info> info_map;
    std::unordered_map <IR::temporary  *,  usage_info >  use_map;

    struct block_info {
        IR::block_stmt *node; /* Current  block. */
        IR::block_stmt *prev; /* Previous block. */
        constant_info  *info; /* Current  info.  */
    };

    std::queue <block_info> block_list;

    constant_propagatior(IR::function *,node *);

    void init_info(IR::function *);

    /* Collect all usage information in all may-go-constant nodes. */
    void collect_use(IR::function *);

    /* Judge whether one node can be constant defed. */
    static bool may_go_constant(IR::node *);

    void update_block();

    void try_spread(constant_info *,IR::node *);

    void spread_state(constant_info *,IR::temporary *);

    void update_constant(IR::function *);

    /* Merge 2 constant info and return whether there is info updated. */
    bool merge_info(IR::block_stmt *,constant_info *);

};

}