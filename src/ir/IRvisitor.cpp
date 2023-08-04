#include "IRvisitor.h"


namespace dark::IR {

void IRbuilder::visitBracketExpr(AST::bracket_expr *) {
    
}

void IRbuilder::visitSubscriptExpr(AST::subscript_expr *) {}
void IRbuilder::visitFunctionExpr(AST::function_expr *) {}
void IRbuilder::visitUnaryExpr(AST::unary_expr *) {}
void IRbuilder::visitMemberExpr(AST::member_expr *) {}
void IRbuilder::visitConstructExpr(AST::construct_expr *) {}
void IRbuilder::visitBinaryExpr(AST::binary_expr *) {}
void IRbuilder::visitConditionExpr(AST::condition_expr *) {}
void IRbuilder::visitAtomExpr(AST::atom_expr *) {}
void IRbuilder::visitLiteralConstant(AST::literal_constant *) {}
void IRbuilder::visitForStmt(AST::for_stmt *) {}
void IRbuilder::visitFlowStmt(AST::flow_stmt *) {}
void IRbuilder::visitWhileStmt(AST::while_stmt *) {}
void IRbuilder::visitBlockStmt(AST::block_stmt *) {}
void IRbuilder::visitBranchStmt(AST::branch_stmt *) {}
void IRbuilder::visitSimpleStmt(AST::simple_stmt *) {}
void IRbuilder::visitVariable(AST::variable_def *) {}
void IRbuilder::visitFunction(AST::function_def *) {}

/* It will go down to member function and member variables. */
void IRbuilder::visitClass(AST::class_def *ctx) {
    auto &__class = class_map[ctx->name];
    for(auto __p : ctx->member) {
        auto __func = dynamic_cast <AST::function_def *> (__p);

        if(__func) visitMemberFunction(__func);




    }
}


void IRbuilder::visitGlobalVariable(AST::variable_def *ctx) {
    for(auto &&[__name,__init] : ctx->init) {
        auto *__var = new global_var;
        __var->name = __name;
        __var->type = type_remap(ctx->type.type);
        literal *__lit;

        switch(__var->type) {
            case typeinfo::I1 : __lit = new boolean_constant;
            case typeinfo::I32: __lit = new integer_constant;
            default:            __lit = new null_constant;
        }

        global_variable.push_back({__var,__lit});

        if(__init) {
            top = &global_init;
            visit(__init);
            auto *__temp = new temporary;
            __temp->name = get_temporary_name(global_init.temp_count);
            __temp->type = type_remap(__init->type);
        }
    }
}

void IRbuilder::visitMemberFunction(AST::function_def *ctx) {
}

}