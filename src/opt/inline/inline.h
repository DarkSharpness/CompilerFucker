#pragma once

#include "optnode.h"
#include "IRbase.h"
#include "IRinfo.h"

#include <bitset>
#include <functional>
#include <unordered_set>

namespace dark::OPT {

/**
 * Pack of 2 pointers.
 * If no exit block, then this function
 * has no call.
 * 
 * 
*/
struct inline_info {
    IR::block_stmt *entry; /* The entry block. */
    IR::block_stmt *exit;  /* The exit block. */
    std::vector <IR::block_stmt *> list;
};

struct inline_visitor final : IR::IRvisitorbase , hidden_impl {
    using _Val_Map = std::unordered_map <IR::definition *,IR::definition *>;
    using _Blk_Map = std::unordered_map <IR::block_stmt *,IR::block_stmt *>;

    inline_info info;
    IR::temporary *dest;

    IR::phi_stmt    *phi;
    IR::block_stmt  *top;
    IR::function   *func;

    _Val_Map val_map;
    _Blk_Map blk_map;

    IR::block_stmt *create_block(IR::block_stmt *);
    IR::definition *copy(IR::definition *);
    inline_visitor(IR::call_stmt *,IR::function *,void *);
    operator inline_info() { return std::move(info); };

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
};


struct recursive_inliner {
    using _Pass = std::function <void(IR::function *,node *)>; 

    /* If a function is inlined too many times, it must be banned! */

    inline static constexpr size_t  BAN_DEPTH = 2;  /* Try only twice. */
    inline static constexpr size_t  MAX_DEPTH = 20; /* If   normal.    */
    using _Bit_Set = std::bitset <MAX_DEPTH>;

    std::unordered_map <IR::function *,_Bit_Set> ban_list;
    size_t depth = 0;


    inline static constexpr size_t  MAX_FUNC  = 600;
    inline static constexpr size_t  MAX_BLOCK = 20;

    inline static constexpr size_t  MIN_FUNC  = 12;
    inline static constexpr size_t  MIN_BLOCK = 4;
    inline static constexpr size_t  SUM_FUNC  = 99;
    inline static constexpr size_t  SUM_BLOCK = 12;

    /* Maximum of function inlined at a time. */
    inline static constexpr size_t  MAX_COUNT = 40;

    recursive_inliner(IR::function *,node *,_Pass &&,void *);
    bool try_inline(IR::function *,void *);
    bool is_inlinable(function_info *,IR::block_stmt *,IR::function *);
    void rebuild_info(IR::function *);
};


}

