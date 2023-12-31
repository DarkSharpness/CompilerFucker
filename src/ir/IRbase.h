#pragma once

#include "IRtype.h"
#include <string>
#include <vector>

namespace dark::IR {

struct IRvisitorbase;

struct node {
    virtual void accept(IRvisitorbase * __v) = 0;
    /* Return the string form IR. */
    virtual std::string data() const = 0;
    /* Return the temporary this statment defines. */
    virtual temporary * get_def() const = 0;
    /* Return all the usages of the node. */
    virtual std::vector <definition *> get_use() const = 0;
    /* Update the old definition with a new one. */
    virtual void update(definition *, definition *) = 0;
    /* Return whether the node is hard undefined behavior. */
    virtual bool is_undefined_behavior() const { return false; }
    virtual ~node() = default;
};

/* Declaration. */
struct block_stmt;
struct function;
struct initialization;
struct compare_stmt;
struct binary_stmt;
struct jump_stmt;
struct branch_stmt;
struct call_stmt;
struct load_stmt;
struct store_stmt;
struct return_stmt;
struct allocate_stmt;
struct get_stmt;
struct phi_stmt;
struct unreachable_stmt;


struct IRvisitorbase {
    void visit(node * __n) { return __n->accept(this); }

    virtual void visitBlock(block_stmt *) = 0;
    virtual void visitFunction(function *) = 0;
    virtual void visitInit(initialization *) = 0;

    virtual void visitCompare(compare_stmt *) = 0;
    virtual void visitBinary(binary_stmt *) = 0;
    virtual void visitJump(jump_stmt *) = 0;
    virtual void visitBranch(branch_stmt *) = 0;
    virtual void visitCall(call_stmt *) = 0;
    virtual void visitLoad(load_stmt *) = 0;
    virtual void visitStore(store_stmt *) = 0;
    virtual void visitReturn(return_stmt *) = 0;
    virtual void visitAlloc(allocate_stmt *) = 0;
    virtual void visitGet(get_stmt *) = 0;
    virtual void visitPhi(phi_stmt *) = 0;
    virtual void visitUnreachable(unreachable_stmt *) = 0;

    virtual ~IRvisitorbase() = default;
};



}