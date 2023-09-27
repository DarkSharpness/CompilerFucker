#pragma once

#include "optnode.h"
#include "IRbase.h"

#include <queue>
#include <unordered_map>

namespace dark::OPT {


struct special_judger final : hidden_impl, IR::IRvisitorbase {
    struct usage_info {
        size_t   use_count  = {};
        IR::node *def_node  = {};
        IR::block_stmt *def_block = {};
    };

    std::unordered_map <IR::temporary *, usage_info> use_map;

    std::unordered_map <IR::call_stmt *,IR::call_stmt *> call_map;
    std::unordered_map <IR::temporary *,IR::definition *> val_map;

    IR::block_stmt *entry_block = {};
    IR::block_stmt *top_block   = {};
    special_judger(IR::function *, node *,void *);

    void visitBlock(IR::block_stmt *) override;
    void visitFunction(IR::function *) override;
    void visitInit(IR::initialization *) override;
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

    void collect_use(IR::function *);
    void replace_function(IR::function *);
    void replace_definition(IR::function *);

};



}