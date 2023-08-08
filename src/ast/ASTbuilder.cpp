#include "ASTbuilder.h"
#include "ASTnode.h"

namespace dark {


/// TODO: Replace this part with visit 3 specific part.
std::any ASTbuilder::visitFile_Input(MxParser::File_InputContext *ctx) {
    for(auto __p : ctx->children) {
        if(__p->getText() == "<EOF>") break;

        auto __v = visit(__p);
        auto __func = std::any_cast <AST::function_def *> (&__v);
        if(__func) {
            global.push_back(*__func);
            continue;
        }

        auto __class = std::any_cast <AST::class_def *> (&__v);
        if(__class) {
            global.push_back(*__class);
            continue;
        }

        auto __var = std::any_cast <AST::variable_def *> (&__v);
        if(__var) {
            global.push_back(*__var);
            continue;
        }
    }
    return nullptr; /* Nothing to return. */
}


// Return a function pointer.
std::any ASTbuilder::visitFunction_Definition(MxParser::Function_DefinitionContext *ctx) {
    auto *__func = new AST::function_def; /* New function qwq. */

    static_cast <AST::argument &> (*__func) 
        = std::any_cast <AST::argument> (visit(ctx->function_Argument()));

    if(ctx->function_Param_List()) {
        __func->args
            = std::any_cast <AST::argument_list> (visit(ctx->function_Param_List()));
    }

    __func->body = // Cast back down to block_stmt pointer.
        static_cast <AST::block_stmt *> (
            std::any_cast <AST::statement *> (visit(ctx->block_Stmt()))
        );

    return __func;
}


// Return a vector of arguments.
std::any ASTbuilder::visitFunction_Param_List(MxParser::Function_Param_ListContext *ctx) {
    AST::argument_list __list; // Return list.
    auto __vec = ctx->function_Argument();
    for(auto __p : __vec)
        __list.push_back(std::any_cast <AST::argument> (visit(__p)));
    return __list;
}


// Return an argument.
std::any ASTbuilder::visitFunction_Argument(MxParser::Function_ArgumentContext *ctx) {
    return AST::argument {
        .name = ctx->Identifier()->getText(),
        .type = std::any_cast <AST::wrapper> (visit(ctx->typename_()))
    };
}


// Return a class pointer.
std::any ASTbuilder::visitClass_Definition(MxParser::Class_DefinitionContext *ctx) {
    auto *__class = new AST::class_def; // Class to return.
    __class->name = ctx->Identifier()->getText();
    auto __vec = ctx->class_Content();
    for(auto __p : __vec) {
        auto __v   = visit(__p);
        auto __tmp = std::any_cast <AST::function_def *> (&__v);
        if(__tmp) __class->member.push_back(*__tmp);
        else { /* Variable definition case. */
            __class->member.push_back(std::any_cast <AST::variable_def *> (__v));
        }
    }
    return __class;
}


// Return a function pointer.
std::any ASTbuilder::visitClass_Ctor_Function(MxParser::Class_Ctor_FunctionContext *ctx) {
    auto *__ctor = new AST::function_def;
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
std::any ASTbuilder::visitClass_Content(MxParser::Class_ContentContext *ctx) {
    if(ctx->variable_Definition()) {
        return visit(ctx->variable_Definition());
    } else if(ctx->function_Definition()) {
        return visit(ctx->function_Definition());
    } else {
        return visit(ctx->class_Ctor_Function());
    }
}


// Return a statement poiner.
std::any ASTbuilder::visitStmt(MxParser::StmtContext *ctx) {
    auto __v = visitChildren(ctx);
    if(!std::any_cast <AST::statement *> (&__v)) {
        return static_cast <AST::statement *> (
            std::any_cast <AST::variable_def *> (__v)
        );
    } else return __v;
}


// Return a statement pointer.
std::any ASTbuilder::visitBlock_Stmt(MxParser::Block_StmtContext *ctx) {
    auto *__block = new AST::block_stmt;
    auto __vec = ctx->stmt();
    for(auto __p : __vec)
        __block->stmt.push_back(std::any_cast <AST::statement *> (visit(__p)));
    return static_cast <AST::statement *> (__block);
}


// Return a statement pointer.
std::any ASTbuilder::visitSimple_Stmt(MxParser::Simple_StmtContext *ctx) {
    auto *__simple = new AST::simple_stmt;
    if(ctx->expr_List())
        __simple->expr = std::any_cast <AST::expression_list> (visit(ctx->expr_List()));
    return static_cast <AST::statement *> (__simple);
}


// Return a statement pointer.
std::any ASTbuilder::visitBranch_Stmt(MxParser::Branch_StmtContext *ctx) {
    auto *__branch = new AST::branch_stmt;
    __branch->data.push_back(
        std::any_cast <AST::branch_stmt::pair_t> (visit(ctx->if_Stmt()))
    );

    auto __vec = ctx->else_if_Stmt();
    for(auto __p : __vec)
        __branch->data.push_back(
            std::any_cast <AST::branch_stmt::pair_t> (visit(__p))
        );

    if(ctx->else_Stmt())
        __branch->data.push_back(
            std::any_cast <AST::branch_stmt::pair_t> (visit(ctx->else_Stmt()))
        );
    
    return static_cast <AST::statement *> (__branch);
}


// Return a pair of condition and body.
std::any ASTbuilder::visitIf_Stmt(MxParser::If_StmtContext *ctx) {
    return AST::branch_stmt::pair_t {
        .cond = std::any_cast <AST::expression *> (visit(ctx->expression())),
        .stmt = std::any_cast <AST::statement  *> (visit(ctx->stmt()))
    };
}


// Return a pair of condition and body.
std::any ASTbuilder::visitElse_if_Stmt(MxParser::Else_if_StmtContext *ctx) {
    return AST::branch_stmt::pair_t {
        .cond = std::any_cast <AST::expression *> (visit(ctx->expression())),
        .stmt = std::any_cast <AST::statement  *> (visit(ctx->stmt()))
    };
}


// Return a pair of condition and body.
std::any ASTbuilder::visitElse_Stmt(MxParser::Else_StmtContext *ctx) {
     return AST::branch_stmt::pair_t {
        .cond = nullptr,
        .stmt = std::any_cast <AST::statement  *> (visit(ctx->stmt()))
    };
}

// Return a statement pointer.
std::any ASTbuilder::visitLoop_Stmt(MxParser::Loop_StmtContext *ctx) {
    if(ctx->for_Stmt()) return visit(ctx->for_Stmt());
    else                return visit(ctx->while_Stmt());
}


// Return a statement pointer.
std::any ASTbuilder::visitFor_Stmt(MxParser::For_StmtContext *ctx) {
    auto *__for = new AST::for_stmt;
    if(ctx->simple_Stmt())
        __for->init = std::any_cast <AST::statement *> (visit(ctx->simple_Stmt()));
    if(ctx->variable_Definition())
        __for->init = std::any_cast <AST::variable_def *> (visit(ctx->variable_Definition()));
    
    if(ctx->condition)
        __for->cond = std::any_cast <AST::expression *> (visit(ctx->condition));
    if(ctx->step)
        __for->step = std::any_cast <AST::expression *> (visit(ctx->step));
    __for->stmt = std::any_cast <AST::statement *> (visit(ctx->stmt()));
    return static_cast <AST::statement *> (__for);
}


// Return a statement pointer.
std::any ASTbuilder::visitWhile_Stmt(MxParser::While_StmtContext *ctx) {
    auto *__while = new AST::while_stmt;
    __while->cond = std::any_cast <AST::expression *> (visit(ctx->expression()));
    __while->stmt = std::any_cast <AST::statement *> (visit(ctx->stmt()));
    return static_cast <AST::statement *> (__while);
}


// Return a statement pointer.
std::any ASTbuilder::visitFlow_Stmt(MxParser::Flow_StmtContext *ctx) {
    auto *__flow = new AST::flow_stmt;
    if(ctx->Continue())     __flow->flow = "continue";
    else if(ctx->Break())   __flow->flow = "break";
    else {
        __flow->flow = "return";
        if(ctx->expression())
            __flow->expr = std::any_cast <AST::expression *> (visit(ctx->expression()));
    }
    return static_cast <AST::statement *> (__flow);
}


// Return a vector of variable pointers.
std::any ASTbuilder::visitVariable_Definition(MxParser::Variable_DefinitionContext *ctx) {
    auto *__var = new AST::variable_def;
    __var->type = std::any_cast <AST::wrapper> (visit(ctx->typename_()));

    auto __vec = ctx->init_Stmt();
    for(auto __p : __vec)
        __var->init.push_back(std::any_cast <AST::variable_def::pair_t> (visit(__p)));

    return __var;
}


// Return a pair of std::string and variable pointer (typename uninitialized).
std::any ASTbuilder::visitInit_Stmt(MxParser::Init_StmtContext *ctx) {
    return AST::variable_def::pair_t {
        ctx->Identifier()->getText(),
        ctx->expression() ?
            std::any_cast <AST::expression *> (visit(ctx->expression())) :
            nullptr
    };
}


// Return a vector of expression pointers
std::any ASTbuilder::visitExpr_List(MxParser::Expr_ListContext *ctx) {
    AST::expression_list __list;
    auto __vec = ctx->expression();
    for(auto __p : __vec)
        __list.push_back(std::any_cast <AST::expression *> (visit(__p)));
    return __list;
}


// Return a expression pointer.
std::any ASTbuilder::visitCondition(MxParser::ConditionContext *ctx) {
    auto *__cond = new AST::condition_expr;
    __cond->cond = std::any_cast <AST::expression *> (visit(ctx->v));
    __cond->lval = std::any_cast <AST::expression *> (visit(ctx->l));
    __cond->rval = std::any_cast <AST::expression *> (visit(ctx->r));
    return static_cast <AST::expression *> (__cond);
}


// Return a expression pointer.
std::any ASTbuilder::visitSubscript(MxParser::SubscriptContext *ctx) {
    auto *__subs = new AST::subscript_expr;
    auto __vec = ctx->expression();
    for(auto __p : __vec)
        __subs->expr.push_back(std::any_cast <AST::expression *> (visit(__p)));
    return static_cast <AST::expression *> (__subs);
}


// Return a expression pointer.
std::any ASTbuilder::visitBinary(MxParser::BinaryContext *ctx) {
    auto *__bin = new AST::binary_expr;
    __bin->lval = std::any_cast <AST::expression *> (visit(ctx->l));
    __bin->rval = std::any_cast <AST::expression *> (visit(ctx->r));
    __bin->op   = ctx->op->getText();
    return static_cast <AST::expression *> (__bin);
}


// Return a expression pointer.
std::any ASTbuilder::visitFunction(MxParser::FunctionContext *ctx) {
    auto *__func = new AST::function_expr;
    __func->body = std::any_cast <AST::expression *> (visit(ctx->l));
    if(ctx->expr_List()) {
        __func->args = std::any_cast <AST::expression_list> (visit(ctx->expr_List()));
    }
    return static_cast <AST::expression *> (__func);
}


// Return a expression pointer.
std::any ASTbuilder::visitBracket(MxParser::BracketContext *ctx) {
    auto *__bra = new AST::bracket_expr;
    __bra->expr = std::any_cast <AST::expression *> (visit(ctx->l));
    return static_cast <AST::expression *> (__bra);
}


// Return a expression pointer.
std::any ASTbuilder::visitMember(MxParser::MemberContext *ctx) {
    auto *__mem = new AST::member_expr;
    __mem->lval = std::any_cast <AST::expression *> (visit(ctx->l));
    __mem->rval = ctx->Identifier()->getText();
    return static_cast <AST::expression *> (__mem);
}


// Return a expression pointer.
std::any ASTbuilder::visitConstruct(MxParser::ConstructContext *ctx) {
    return visit(ctx->new_Type());
}


// Return a expression pointer.
std::any ASTbuilder::visitUnary(MxParser::UnaryContext *ctx) {
    auto *__una = new AST::unary_expr;
    if(ctx->l) {
        __una->expr = std::any_cast <AST::expression *> (visit(ctx->l));
        __una->op   = ctx->op->getText();
        __una->op[2]= __una->op[0];
    } else {
        __una->expr = std::any_cast <AST::expression *> (visit(ctx->r));
        __una->op   = ctx->op->getText();
    }
    return static_cast <AST::expression *> (__una);
}


// Return a expression pointer.
std::any ASTbuilder::visitAtom(MxParser::AtomContext *ctx) {
    auto *__atom = new AST::atom_expr;
    __atom->name = ctx->Identifier()->getText();
    return static_cast <AST::expression *> (__atom);
}


// Return a expression pointer.
std::any ASTbuilder::visitLiteral(MxParser::LiteralContext *ctx) {
    auto *__lite = new AST::literal_constant;
    auto *__temp = ctx->literal_Constant();

    __lite->name = __temp->getText();
    if(__temp->Number())        __lite->type = AST::literal_constant::NUMBER;
    else if(__temp->Cstring()) {
        __lite->type = AST::literal_constant::CSTRING;
        __lite->name = Mx_string_parse(std::move(__lite->name));
    }
    else if(__temp->Null())     __lite->type = AST::literal_constant::NULL_;
    else if(__temp->True())     __lite->type = AST::literal_constant::TRUE;
    else                        __lite->type = AST::literal_constant::FALSE;

    return static_cast <AST::expression *> (__lite);
}


// Return specific typeinfo wrapper.
std::any ASTbuilder::visitTypename(MxParser::TypenameContext *ctx) {
    int __dim = ctx->Brack_Left_().size();
    if(ctx->Identifier()) {
        return AST::wrapper {
            .type = get_typeinfo(ctx->Identifier()->getText()),
            .info = __dim, /* Dimension.  */
            .flag = 0, /* Not assignable. */
        };
    } else {
        return AST::wrapper {
            .type = get_typeinfo(ctx->BasicTypes()->getText()),
            .info = __dim, /* Dimension.  */
            .flag = 0, /* Not assignable. */
        };
    }
}


// Return a expression pointer.
std::any ASTbuilder::visitNew_Type(MxParser::New_TypeContext *ctx) {
    auto *__new = new AST::construct_expr;
    int   __dim = 0; /* Default as 0 dimensions. */

    if(ctx->new_Index()) {
        if(ctx->Paren_Right())
            throw error("No parenthesis should be applied to new array expression!");
        auto &&__tmp = std::any_cast <
            std::pair <AST::expression_list,size_t>
        > (visit(ctx->new_Index()));
        __new->expr = std::move(__tmp.first);
        __dim       = __tmp.second;
    }

    if(ctx->Identifier()) {
        __new->type = AST::wrapper {
            .type = get_typeinfo(ctx->Identifier()->getText()),
            .info = __dim, /* Dimension.  */
            .flag = 0, /* Not assignable. */
        };
    } else {
        __new->type = AST::wrapper {
            .type = get_typeinfo(ctx->BasicTypes()->getText()),
            .info = __dim, /* Dimension.  */
            .flag = 0, /* Not assignable. */
        };
    }
    return static_cast <AST::expression *> (__new);
}


// Return a vector of expression pointers and total dimensions.
std::any ASTbuilder::visitNew_Index(MxParser::New_IndexContext *ctx) {
    AST::expression_list __list;
    if(!ctx->bad.empty()) throw error("Bad new expression!");
    auto __vec = ctx->good;
    for(auto __p : __vec)
        __list.push_back(std::any_cast <AST::expression *> (visit(__p)));
    return std::make_pair(std::move(__list),ctx->Brack_Left_().size());
}


// It won't be reached!
std::any ASTbuilder::visitLiteral_Constant(MxParser::Literal_ConstantContext *ctx) {
    throw error("Literal constant should never be reached!");
}

std::any ASTbuilder::visitThis(MxParser::ThisContext *context) {
    auto *__atom = new AST::atom_expr;
    __atom->name = "this";
    return static_cast <AST::expression *> (__atom);
}


}