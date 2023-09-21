#pragma once

#include "optnode.h"
#include <vector>
#include <deque>

namespace dark::OPT {


struct constant_calculator final : IR::IRvisitorbase {
    inline static std::deque <IR::global_variable> generated{};
    void *data; /* This argument is responsible for passing the params. */

    static IR::global_variable *generate_string(std::string __str) {
        for(auto &__glo : generated) {
            auto &__tmp = safe_cast <IR::string_constant *> 
                (__glo.const_val)->context;
            if (__tmp == __str) return &__glo;
        }

        auto *__lit = new IR::string_constant {std::move(__str)};
        auto *__global = &generated.emplace_back();
        __global->name = "@str.gen." + std::to_string(generated.size());
        __global->type = {&IR::__string_class__ ,1};
        __global->const_val = __lit;
        return __global;
    }

    using array_type = std::vector<IR::definition *>;
    explicit constant_calculator() = default;

    IR::string_constant *try_get_string(IR::definition *def);

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