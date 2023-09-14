#pragma once

#include "optnode.h"
#include <vector>

namespace dark::OPT {


struct constant_calculator final : IR::IRvisitorbase {
    void *data; /* This argument is responsible for passing the params. */
    using array_type = std::vector<IR::definition *>;
    explicit constant_calculator() = default;

    const array_type &get_array()
    { return *static_cast <const array_type *> (data); }

    explicit operator IR::definition *()
    { return static_cast <IR::definition *> (data); }

    void set_array(array_type &__vec)
    { data = static_cast <void *> (&__vec); }

    void set_result(IR::definition *__def) 
    { data = static_cast <void *> (__def); }
 
    /* Work out the const folding result. */
    [[nodiscard]] inline IR::definition *
        work(IR::node *__ctx,array_type &__vec) {
        set_array(__vec);
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

}