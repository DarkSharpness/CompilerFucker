#include "ASTvisitor.h"
#include "ASTnode.h"

namespace dark {


std::any ASTvisitor::visitFile_Input(MxParser::File_InputContext *ctx) {
    for(auto __p : ctx->children) {
        auto __v = visit(__p);
        auto __tmp = std::any_cast <AST::node *> (&__v);
        if(!__tmp) throw error("File input fucked!");
        global.push_back(*__tmp);
    }
}

std::any ASTvisitor::visitFunction_Definition(MxParser::Function_DefinitionContext *ctx) {
    auto *__func = new AST::function; /* New function qwq. */

    auto __v = visit(ctx->function_Argument());
    try {
        __func->info = std::move(std::any_cast <AST::argument> (__v));
        __func->name = __func->info.name;
    } catch(...) {
        throw error("Invalid function definition return type.");
    }

    if(ctx->function_Param_List()) {
        __v = visit(ctx->function_Param_List());
        try {
            __func->arg_list = std::move(std::any_cast <std::vector <AST::argument>> (__v));
        } catch(...) {
            throw error("Invalid function argument list!");
        }
    }

    __v = visit(ctx->block_Stmt());
    try {
        __func->body = std::any_cast <AST::block_stmt *> (__v);
    } catch(...) {
        throw error("Invalid function body block!");
    }
    return __func;
}

std::any ASTvisitor::visitFunction_Param_List(MxParser::Function_Param_ListContext *ctx) {
    auto __vec = ctx->function_Argument();
    std::vector <AST::argument> __ans;
    for(auto __p : __vec) {
        auto __v = visit(__p);
        __ans.push_back(std::move(std::any_cast <AST::argument> (__p)));
    }
    return __ans;
}

std::any ASTvisitor::visitFunction_Argument(MxParser::Function_ArgumentContext *ctx) {
    auto __v = visit(ctx->typename_());
    return AST::argument {
        .type = std::any_cast <AST::wrapper> (__v),
        .name = ctx->Identifier()->getText()
    };
}

std::any ASTvisitor::visitClass_Definition(MxParser::Class_DefinitionContext *ctx) {
    auto *__class = new AST::object;
    __class->name = ctx->Identifier()->getText();
    auto __v = visit(ctx->class_Content());
    

}

std::any ASTvisitor::visitClass_Ctor_Function(MxParser::Class_Ctor_FunctionContext *ctx) {};

std::any ASTvisitor::visitClass_Content(MxParser::Class_ContentContext *ctx) {};

std::any ASTvisitor::visitStmt(MxParser::StmtContext *ctx) {};

std::any ASTvisitor::visitBlock_Stmt(MxParser::Block_StmtContext *ctx) {};

std::any ASTvisitor::visitSimple_Stmt(MxParser::Simple_StmtContext *ctx) {};

std::any ASTvisitor::visitBranch_Stmt(MxParser::Branch_StmtContext *ctx) {};

std::any ASTvisitor::visitIf_Stmt(MxParser::If_StmtContext *ctx) {};

std::any ASTvisitor::visitElse_if_Stmt(MxParser::Else_if_StmtContext *ctx) {};

std::any ASTvisitor::visitElse_Stmt(MxParser::Else_StmtContext *ctx) {};

std::any ASTvisitor::visitLoop_Stmt(MxParser::Loop_StmtContext *ctx) {};

std::any ASTvisitor::visitFor_Stmt(MxParser::For_StmtContext *ctx) {};

std::any ASTvisitor::visitWhile_Stmt(MxParser::While_StmtContext *ctx) {};

std::any ASTvisitor::visitFlow_Stmt(MxParser::Flow_StmtContext *ctx) {};

std::any ASTvisitor::visitVariable_Definition(MxParser::Variable_DefinitionContext *ctx) {};

std::any ASTvisitor::visitInit_Stmt(MxParser::Init_StmtContext *ctx) {};

std::any ASTvisitor::visitExpr_List(MxParser::Expr_ListContext *ctx) {};

std::any ASTvisitor::visitCondition(MxParser::ConditionContext *ctx) {};

std::any ASTvisitor::visitSubscript(MxParser::SubscriptContext *ctx) {};

std::any ASTvisitor::visitBinary(MxParser::BinaryContext *ctx) {};

std::any ASTvisitor::visitFunction(MxParser::FunctionContext *ctx) {};

std::any ASTvisitor::visitBracket(MxParser::BracketContext *ctx) {};

std::any ASTvisitor::visitMember(MxParser::MemberContext *ctx) {};

std::any ASTvisitor::visitConstruct(MxParser::ConstructContext *ctx) {};

std::any ASTvisitor::visitUnary(MxParser::UnaryContext *ctx) {};

std::any ASTvisitor::visitAtom(MxParser::AtomContext *ctx) {};

std::any ASTvisitor::visitLiteral(MxParser::LiteralContext *ctx) {};

std::any ASTvisitor::visitTypename(MxParser::TypenameContext *ctx) {};

std::any ASTvisitor::visitNew_Type(MxParser::New_TypeContext *ctx) {};

std::any ASTvisitor::visitNew_Index(MxParser::New_IndexContext *ctx) {};

std::any ASTvisitor::visitLiteral_Constant(MxParser::Literal_ConstantContext *ctx) {};



}