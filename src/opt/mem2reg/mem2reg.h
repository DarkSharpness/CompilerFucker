#pragma once

#include "optnode.h"
#include "IRnode.h"
#include "IRbase.h"

#include <deque>


/* Optimize only. */
namespace dark::OPT {

struct SSAbuilder final : IR::IRvisitorbase {
    std::map <IR::block_stmt *,node> node_map;

    node *top;
    size_t end_tag = 0;
    node *create_node(IR::block_stmt *__block) {
        return &node_map.try_emplace(__block,__block).first->second;
    }

    SSAbuilder
        (std::vector <IR::initialization> &global_variables,
         std::vector <IR::function   >    &global_functions) {
        for(auto &__func : global_functions)
            visitFunction(&__func);

        try_optimize(global_functions);

        update_pool(global_variables);
    }

    void try_optimize(std::vector <IR::function> &);
    void update_pool(std::vector <IR::initialization> &);

    void visitBlock(IR::block_stmt*) override;
    void visitFunction(IR::function*) override;
    void visitInit(IR::initialization*) override;

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

    void reset_CFG(IR::function *__func);
    void rebuild_CFG(IR::function *__func);
    void reverse_CFG(IR::function *__func);
    void reset_CFG(IR::block_stmt *__block);

    /* Must be inlined! */
    inline void link(node *from, node *to) {
        from->next.push_back(to);
        to->prev.push_back(from);
    }

    void debug_print(IR::function *ctx,std::ostream &os = std::cerr) {
        os << "Function: " << ctx->name << "\n";
        os << "  Blocks:\n";
        for(auto __p : ctx->stmt)
            os << create_node(__p)->data() << '\n';
    }

    virtual ~SSAbuilder() = default;
};


}
