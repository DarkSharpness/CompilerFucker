#pragma once

#include "ASTscanner.h"


namespace dark::IR {


struct IRvisitor : AST::ASTvisitorbase {
    scope *global = nullptr;

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

};

}
