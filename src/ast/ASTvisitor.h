#pragma once

#include "../generated/MxParserVisitor.h"
#include "utility.h"

namespace dark {

class ASTvisitor : MxParserVisitor {
  public:
    std::any visitFile_Input(MxParser::File_InputContext *context) override;

    std::any visitFunction_Definition(MxParser::Function_DefinitionContext *context) override;

    std::any visitFunction_Param_List(MxParser::Function_Param_ListContext *context) override;

    std::any visitFunction_Argument(MxParser::Function_ArgumentContext *context) override;

    std::any visitClass_Definition(MxParser::Class_DefinitionContext *context) override;

    std::any visitClass_Ctor_Function(MxParser::Class_Ctor_FunctionContext *context) override;

    std::any visitClass_Content(MxParser::Class_ContentContext *context) override;

    std::any visitStmt(MxParser::StmtContext *context) override;

    std::any visitBlock_Stmt(MxParser::Block_StmtContext *context) override;

    std::any visitSimple_Stmt(MxParser::Simple_StmtContext *context) override;

    std::any visitBranch_Stmt(MxParser::Branch_StmtContext *context) override;

    std::any visitIf_Stmt(MxParser::If_StmtContext *context) override;

    std::any visitElse_if_Stmt(MxParser::Else_if_StmtContext *context) override;

    std::any visitElse_Stmt(MxParser::Else_StmtContext *context) override;

    std::any visitLoop_Stmt(MxParser::Loop_StmtContext *context) override;

    std::any visitFor_Stmt(MxParser::For_StmtContext *context) override;

    std::any visitWhile_Stmt(MxParser::While_StmtContext *context) override;

    std::any visitFlow_Stmt(MxParser::Flow_StmtContext *context) override;

    std::any visitVariable_Definition(MxParser::Variable_DefinitionContext *context) override;

    std::any visitInit_Stmt(MxParser::Init_StmtContext *context) override;

    std::any visitExpr_List(MxParser::Expr_ListContext *context) override;

    std::any visitCondition(MxParser::ConditionContext *context) override;

    std::any visitSubscript(MxParser::SubscriptContext *context) override;

    std::any visitBinary(MxParser::BinaryContext *context) override;

    std::any visitFunction(MxParser::FunctionContext *context) override;

    std::any visitBracket(MxParser::BracketContext *context) override;

    std::any visitMember(MxParser::MemberContext *context) override;

    std::any visitUnary(MxParser::UnaryContext *context) override;

    std::any visitAtom(MxParser::AtomContext *context) override;

    std::any visitLiteral(MxParser::LiteralContext *context) override;

    std::any visitTypename(MxParser::TypenameContext *context) override;

    std::any visitNew_Type(MxParser::New_TypeContext *context) override;

    std::any visitNew_Index(MxParser::New_IndexContext *context) override;

    std::any visitLiteral_Constant(MxParser::Literal_ConstantContext *context) override;
};




}