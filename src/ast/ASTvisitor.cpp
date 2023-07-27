#include "ASTvisitor.h"
#include "ASTnode.h"

namespace dark {

/// TODO: Replace this part with visit 3 specific part.
std::any ASTvisitor::visitFile_Input(MxParser::File_InputContext *ctx) {
    for(auto __p : ctx->children) {
        auto __v = visit(__p);
        auto __func = std::any_cast <AST::function *> (&__v);
        if(__func) {
            global.push_back(*__func);
            continue;
        }

        auto __class = std::any_cast <AST::object *> (&__v);
        if(__class) {
            global.push_back(*__class);
            continue;
        }

        auto __var_list = std::any_cast <std::vector <AST::variable *>> (&__v);
        if(__var_list) {
            for(auto __var : *__var_list)
                global.push_back(__var);
            continue;
        }

        throw error("File input error!");
    }
}

// Return a function pointer.
std::any ASTvisitor::visitFunction_Definition(MxParser::Function_DefinitionContext *ctx) {
    auto *__func = new AST::function; /* New function qwq. */

    static_cast <AST::argument &> (*__func) 
        = std::any_cast <AST::argument> (visit(ctx->function_Argument()));

    if(ctx->function_Param_List()) {
        __func->arg_list
            = std::any_cast <std::vector <AST::argument>> (visit(ctx->function_Param_List()));
    }

    __func->body = // Cast back down to block_stmt pointer.
        static_cast <AST::block_stmt *> (
            std::any_cast <AST::statement *> (visit(ctx->block_Stmt()))
        );

    return __func;
}

// Return a vector of arguments.
std::any ASTvisitor::visitFunction_Param_List(MxParser::Function_Param_ListContext *ctx) {
    std::vector <AST::argument> __list; // Return list.
    auto __vec = ctx->function_Argument();
    for(auto __p : __vec)
        __list.push_back(std::any_cast <AST::argument> (visit(__p)));
    return __list;
}

// Return an argument.
std::any ASTvisitor::visitFunction_Argument(MxParser::Function_ArgumentContext *ctx) {
    return AST::argument {
        .name = ctx->Identifier()->getText(),
        .type = std::any_cast <AST::wrapper> (visit(ctx->typename_()))
    };
}

// Return a class pointer.
std::any ASTvisitor::visitClass_Definition(MxParser::Class_DefinitionContext *ctx) {
    auto *__class = new AST::object; // Class to return.
    __class->name = ctx->Identifier()->getText();
    auto __vec = ctx->class_Content();
    for(auto __p : __vec) {
        auto __v   = visit(__p);
        auto __tmp = std::any_cast <AST::function *> (&__v);
        if(__tmp) __class->member.push_back(*__tmp);
        else { /* Variable definition case. */
            auto __var_list = std::any_cast <std::vector <AST::variable *>> (__v);
            for(auto __var : __var_list)
                __class->member.push_back(__var);
        }
    }
    return __class;
}

// Return a function pointer.
std::any ASTvisitor::visitClass_Ctor_Function(MxParser::Class_Ctor_FunctionContext *ctx) {
    auto *__ctor = new AST::function;
    /* __ctor has no name, currently no arg_list. */
    __ctor->type = AST::wrapper {
        .type = get_typeinfo(ctx->Identifier()->getText()),
        .info = 0, /* Dimension. */
        .flag = 0, /* Result: rvalue(not assignable) */
    };
    __ctor->body = // Cast back down to block_stmt pointer.
        static_cast <AST::block_stmt *> (
            std::any_cast <AST::statement *> (visit(ctx->block_Stmt()))
        );

    return __ctor;
}


// Return a function pointer or vector of variable pointers.
std::any ASTvisitor::visitClass_Content(MxParser::Class_ContentContext *ctx) {
    if(ctx->variable_Definition()) {
        return visit(ctx->variable_Definition());
    } else if(ctx->function_Definition()) {
        return visit(ctx->function_Definition());
    } else {
        return visit(ctx->class_Ctor_Function());
    }
}


// Return a statement poiner.
std::any ASTvisitor::visitStmt(MxParser::StmtContext *ctx) {

}


// Return a statement pointer.
std::any ASTvisitor::visitBlock_Stmt(MxParser::Block_StmtContext *ctx) {
    auto *__block = new AST::block_stmt;
    auto __vec = ctx->stmt();
    for(auto __p : __vec)
        __block->stmt.push_back(std::any_cast <AST::statement *> (visit(__p)));
    return static_cast <AST::statement *> (__block);
}


// Return a statement pointer.
std::any ASTvisitor::visitSimple_Stmt(MxParser::Simple_StmtContext *ctx) {
    auto *__simple = new AST::simple_stmt;
    if(ctx->expr_List())
        __simple->expr =
            std::any_cast <std::vector <AST::expression *>> 
                (visit(ctx->expr_List()));
    return static_cast <AST::statement *> (__simple);
}


// Return a statement pointer.
std::any ASTvisitor::visitBranch_Stmt(MxParser::Branch_StmtContext *ctx) {
    auto *__branch = new AST::branch_stmt;

    __branch->data.push_back(
        std::any_cast <AST::branch_stmt::pair_t> (ctx->if_Stmt())
    );

    auto __vec = ctx->else_if_Stmt();
    for(auto __p : __vec)
        __branch->data.push_back(
            std::any_cast <AST::branch_stmt::pair_t> (__p)
        );

    if(ctx->else_Stmt())
        __branch->data.push_back(
            std::any_cast <AST::branch_stmt::pair_t> (ctx->else_Stmt())
        );
    
    return static_cast <AST::statement *> (__branch);
}


// Return a pair of condition and body.
std::any ASTvisitor::visitIf_Stmt(MxParser::If_StmtContext *ctx) {
    return AST::branch_stmt::pair_t {
        .cond = std::any_cast <AST::expression *> (visit(ctx->expression())),
        .stmt = std::any_cast <AST::statement  *> (visit(ctx->stmt()))
    };
}


// Return a pair of condition and body.
std::any ASTvisitor::visitElse_if_Stmt(MxParser::Else_if_StmtContext *ctx) {
    return AST::branch_stmt::pair_t {
        .cond = std::any_cast <AST::expression *> (visit(ctx->expression())),
        .stmt = std::any_cast <AST::statement  *> (visit(ctx->stmt()))
    };
}


// Return a pair of condition and body.
std::any ASTvisitor::visitElse_Stmt(MxParser::Else_StmtContext *ctx) {
     return AST::branch_stmt::pair_t {
        .cond = nullptr,
        .stmt = std::any_cast <AST::statement  *> (visit(ctx->stmt()))
    };
}

// Return a statement pointer.
std::any ASTvisitor::visitLoop_Stmt(MxParser::Loop_StmtContext *ctx) {
    if(ctx->for_Stmt()) visit(ctx->for_Stmt());
    else                visit(ctx->while_Stmt());
}


// Return a statement pointer.
std::any ASTvisitor::visitFor_Stmt(MxParser::For_StmtContext *ctx) {
    auto *__for = new AST::for_stmt;
    if(ctx->start)
        __for->init = std::any_cast <AST::expression *> (visit(ctx->start));
    if(ctx->condition)
        __for->cond = std::any_cast <AST::expression *> (visit(ctx->condition));
    if(ctx->step)
        __for->step = std::any_cast <AST::expression *> (visit(ctx->step));
    __for->stmt = std::any_cast <AST::statement *> (visit(ctx->stmt()));
    return static_cast <AST::statement *> (__for);
}


// Return a statement pointer.
std::any ASTvisitor::visitWhile_Stmt(MxParser::While_StmtContext *ctx) {
    auto *__while = new AST::while_stmt;
    __while->cond = std::any_cast <AST::expression *> (visit(ctx->expression()));
    __while->stmt = std::any_cast <AST::statement *> (visit(ctx->stmt()));
    return static_cast <AST::statement *> (__while);
}


// Return a statement pointer.
std::any ASTvisitor::visitFlow_Stmt(MxParser::Flow_StmtContext *ctx) {
    auto *__flow = new AST::flow_stmt;
    if(ctx->Continue())     __flow->flow = "continue";
    else if(ctx->Break())   __flow->flow = "break";
    else {
        __flow->flow = "return";
        __flow->expr = std::any_cast <AST::expression *> (visit(ctx->expression()));
    }
    return static_cast <AST::statement *> (__flow);
}


// Return a vector of variable pointers.
std::any ASTvisitor::visitVariable_Definition(MxParser::Variable_DefinitionContext *ctx) {
    std::vector <AST::variable *> __list;
    auto __vec = ctx->init_Stmt();
    for(auto __p : __vec)
        __list.push_back(std::any_cast <AST::variable *> (visit(__p)));
    return __list;
}


// Return a variable pointer (typename uninitialized).
std::any ASTvisitor::visitInit_Stmt(MxParser::Init_StmtContext *ctx) {
    auto *__var = new AST::variable;
    __var->name = ctx->Identifier()->getText();
    if(ctx->expression())
        __var->init = std::any_cast <AST::expression *> (visit(ctx->expression()));
    return __var;
}


// Return a vector of expression pointers
std::any ASTvisitor::visitExpr_List(MxParser::Expr_ListContext *ctx) {
    std::vector <AST::expression *> __list;
    auto __vec = ctx->expression();
    for(auto __p : __vec)
        __list.push_back(std::any_cast <AST::expression *> (visit(__p)));
    return __list;
}


// Return a expression pointer.
std::any ASTvisitor::visitCondition(MxParser::ConditionContext *ctx) {
    auto *__cond = new AST::condition_expr;
    __cond->cond = std::any_cast <AST::expression *> (visit(ctx->v));
    __cond->lval = std::any_cast <AST::expression *> (visit(ctx->l));
    __cond->rval = std::any_cast <AST::expression *> (visit(ctx->r));
    return static_cast <AST::expression *> (__cond);
}


// Return a expression pointer.
std::any ASTvisitor::visitSubscript(MxParser::SubscriptContext *ctx) {
    auto *__subs = new AST::subscript_expr;
    auto __vec = ctx->expression();
    for(auto __p : __vec)
        __subs->expr.push_back(std::any_cast <AST::expression *> (visit(__p)));
    return static_cast <AST::expression *> (__subs);
}


// Return a expression pointer.
std::any ASTvisitor::visitBinary(MxParser::BinaryContext *ctx) {
    auto *__bin = new AST::binary_expr;
    __bin->lval = std::any_cast <AST::expression *> (visit(ctx->l));
    __bin->rval = std::any_cast <AST::expression *> (visit(ctx->r));
    __bin->op   = ctx->op->getText();
    return static_cast <AST::expression *> (__bin);
}


// Return a expression pointer.
std::any ASTvisitor::visitFunction(MxParser::FunctionContext *ctx) {
    auto *__func = new AST::function_expr;
    __func->body = std::any_cast <AST::expression *> (visit(ctx->l));
    if(ctx->expr_List()) {
        __func->args = 
            std::any_cast <std::vector <AST::expression *>> 
                (visit(ctx->expr_List()));
    }
    
}


// Return a expression pointer.
std::any ASTvisitor::visitBracket(MxParser::BracketContext *ctx) {

}


// Return a expression pointer.
std::any ASTvisitor::visitMember(MxParser::MemberContext *ctx) {
    
}


// Return a expression pointer.
std::any ASTvisitor::visitConstruct(MxParser::ConstructContext *ctx) {

}


// Return a expression pointer.
std::any ASTvisitor::visitUnary(MxParser::UnaryContext *ctx) {

}


// Return a expression pointer.
std::any ASTvisitor::visitAtom(MxParser::AtomContext *ctx) {

}


// Return a expression pointer.
std::any ASTvisitor::visitLiteral(MxParser::LiteralContext *ctx) {

}


// Return specific typeinfo.
std::any ASTvisitor::visitTypename(MxParser::TypenameContext *ctx) {

}


// Return specific typeinfo.
std::any ASTvisitor::visitNew_Type(MxParser::New_TypeContext *ctx) {

}


// Return the expression in the index.
std::any ASTvisitor::visitNew_Index(MxParser::New_IndexContext *ctx) {
    
}


// It won't be reached!
std::any ASTvisitor::visitLiteral_Constant(MxParser::Literal_ConstantContext *ctx) {
    throw error("Literal constant should never be reached!");
}



}