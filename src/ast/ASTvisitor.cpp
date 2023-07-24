#include "ASTvisitor.h"
#include "ASTnode.h"

namespace dark {


std::any ASTvisitor::visitFile_Input(MxParser::File_InputContext *ctx) {
    for(auto &&__ch : ctx->children) {
        auto result = visit(__ch);
        std::any_cast <int> (result);
    }
}

std::any ASTvisitor::visitFunction_Definition(MxParser::Function_DefinitionContext *ctx) {


}

std::any ASTvisitor::visitFunction_Param_List(MxParser::Function_Param_ListContext *ctx){}

std::any ASTvisitor::visitFunction_Argument(MxParser::Function_ArgumentContext *ctx){}

std::any ASTvisitor::visitClass_Definition(MxParser::Class_DefinitionContext *ctx){}

std::any ASTvisitor::visitClass_Ctor_Function(MxParser::Class_Ctor_FunctionContext *ctx){}

std::any ASTvisitor::visitClass_Content(MxParser::Class_ContentContext *ctx){}

std::any ASTvisitor::visitStmt(MxParser::StmtContext *ctx){}

std::any ASTvisitor::visitBlock_Stmt(MxParser::Block_StmtContext *ctx){}

std::any ASTvisitor::visitSimple_Stmt(MxParser::Simple_StmtContext *ctx){}

std::any ASTvisitor::visitBranch_Stmt(MxParser::Branch_StmtContext *ctx){}

std::any ASTvisitor::visitIf_Stmt(MxParser::If_StmtContext *ctx){}

std::any ASTvisitor::visitElse_if_Stmt(MxParser::Else_if_StmtContext *ctx){}

std::any ASTvisitor::visitElse_Stmt(MxParser::Else_StmtContext *ctx){}

std::any ASTvisitor::visitLoop_Stmt(MxParser::Loop_StmtContext *ctx){}

std::any ASTvisitor::visitFor_Stmt(MxParser::For_StmtContext *ctx){}

std::any ASTvisitor::visitWhile_Stmt(MxParser::While_StmtContext *ctx){}

std::any ASTvisitor::visitFlow_Stmt(MxParser::Flow_StmtContext *ctx){}

std::any ASTvisitor::visitVariable_Definition(MxParser::Variable_DefinitionContext *ctx){}

std::any ASTvisitor::visitInit_Stmt(MxParser::Init_StmtContext *ctx){}

std::any ASTvisitor::visitExpr_List(MxParser::Expr_ListContext *ctx){}

std::any ASTvisitor::visitExpression(MxParser::ExpressionContext *ctx){}

std::any ASTvisitor::visitTypename(MxParser::TypenameContext *ctx){}

std::any ASTvisitor::visitNew_Type(MxParser::New_TypeContext *ctx){}

std::any ASTvisitor::visitNew_Index(MxParser::New_IndexContext *ctx){}

std::any ASTvisitor::visitLiteral_Constant(MxParser::Literal_ConstantContext *ctx){}

}