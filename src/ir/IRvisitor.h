#pragma once

#include "utility.h"
#include "IRnode.h"

namespace dark::IR {


struct IRbuilder : AST::ASTvisitorbase {
    scope *global = nullptr;

    std::map <std::string,IR::class_type> class_map;

    function global_init; /* Global init function. */
    std::vector <initialization> global_variable;

    function *top = nullptr;

    IR::typeinfo type_remap(AST::typeinfo *type) {
        if(type->name == "int")  return IR::typeinfo::I32;
        if(type->name == "bool") return IR::typeinfo::I1;
        return IR::typeinfo::PTR;
    }

    IRbuilder(scope *__global) : global(__global) {
        global_init.emplace_new(new block_stmt);
        global_init.stmt.back()->label = "entry";
    }


    void visitBracketExpr(AST::bracket_expr *) override;
    void visitSubscriptExpr(AST::subscript_expr *) override;
    void visitFunctionExpr(AST::function_expr *) override;
    void visitUnaryExpr(AST::unary_expr *) override;
    void visitMemberExpr(AST::member_expr *) override;
    void visitConstructExpr(AST::construct_expr *) override;
    void visitBinaryExpr(AST::binary_expr *) override;
    void visitConditionExpr(AST::condition_expr *) override;
    void visitAtomExpr(AST::atom_expr *) override;
    void visitLiteralConstant(AST::literal_constant *) override;
    void visitForStmt(AST::for_stmt *) override;
    void visitFlowStmt(AST::flow_stmt *) override;
    void visitWhileStmt(AST::while_stmt *) override;
    void visitBlockStmt(AST::block_stmt *) override;
    void visitBranchStmt(AST::branch_stmt *) override;
    void visitSimpleStmt(AST::simple_stmt *) override;
    void visitVariable(AST::variable_def *) override;
    void visitFunction(AST::function_def *) override;
    void visitClass(AST::class_def *) override;

    void visitGlobalVariable(AST::variable_def *);
    void visitMemberFunction(AST::function_def *);

    static std::string get_temporary_name(size_t __n)
    { return "%" + std::to_string(__n); }

};

}
