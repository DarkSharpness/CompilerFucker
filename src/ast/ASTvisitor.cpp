#include "ASTvisitor.h"


namespace dark::AST {


void ASTvisitor::visitBracketExpr(bracket_expr *ctx) {
    ctx->space = top;
    visit(ctx->expr);
    static_cast <wrapper &> (*ctx) = *ctx->expr;
}


void ASTvisitor::visitSubscriptExpr(subscript_expr *ctx) {
    ctx->space = top;
    for(size_t i = 1 ; i < ctx->expr.size() ; ++i) {
        auto __p = ctx->expr[i];
        visit(__p);
        if(!__p->check("int",0))
            throw error("Non-integer subscript!",ctx);
    }
    auto __l = ctx->expr[0];
    visit(__l);

    static_cast <wrapper &> (*ctx) = *__l;
    ctx->flag = true; /* Array access are re-assignable. */
    ctx->info -= (ctx->expr.size() - 1);
    if(ctx->info < 0) throw error("Too many subscripts!",ctx);
}


void ASTvisitor::visitFunctionExpr(function_expr *ctx) {
    ctx->space = top;
    /* Check for function type. */
    visit(ctx->body);
    const wrapper &__type = *ctx->body;
    if(!__type.type->is_function())
        throw error("Unknown function: \"" + __type.name() + '\"',ctx->body);
    auto *__func = __type.type->func;
    if(__func->args.size() != ctx->args.size())
        throw error("Function params length dismatch!",ctx);

    /* Check for function arguments. */
    for(size_t i = 0 ; i < ctx->args.size() ; ++i) {
        auto __p = ctx->args[i];
        visit(__p);
        if(!is_convertible(*__p,__func->args[i].type))
            throw error(
                "Function params dismatch: "
                "Cannot convert type \"" 
                + __p->data()
                + "\" to \""
                + __func->args[i].type.data()
                + "\".",ctx
            );
    }

    static_cast <wrapper &> (*ctx) = __func->type;
}


void ASTvisitor::visitUnaryExpr(unary_expr *ctx) {
    ctx->space = top;
    visit(ctx->expr);
    if(ctx->op[1]) { /* ++ or -- */
        if(!ctx->expr->check("int",0)) {
            throw error(
                std::string("No such operator ")
                + ctx->op.str
                + " for type \""
                + ctx->expr->name()
                + "\".",ctx
            );
        }
        if(!ctx->expr->assignable())
            throw error("No self-iteration for right value type.",ctx);

        static_cast <wrapper &> (*ctx) = *ctx->expr;
        ctx->flag = !ctx->op[2];
    } else {
        if(((ctx->op[0] == '+' || ctx->op[0] == '-') && !ctx->expr->check("int",0))
        || ((ctx->op[0] == '!' || ctx->op[0] == '~') && !ctx->expr->check("bool",0))) {
            throw error(
                std::string("No such operator ")
                + ctx->op.str
                + " for type \""
                + ctx->expr->name()
                + "\".",ctx
            );
        }

        /* Of course not assignable. */
        static_cast <wrapper &> (*ctx) = *ctx->expr;
        ctx->flag = false;
    }

}


void ASTvisitor::visitMemberExpr(member_expr *ctx) {
    ctx->space = top;
    visit(ctx->lval);

    const wrapper &__type = *ctx->lval;
    if(__type.type->is_function())
        throw error("Function doesn't possess scope!",ctx);
    if(__type.dimension()) {
        /* Actually, only size method is ok. */
        auto __p = class_map["__array__"].space->find(ctx->rval);
        if (!__p) /* Not found in array space. */
            throw error( "Member \"" + ctx->rval + "\" not found in array type",ctx);
        static_cast <wrapper &> (*ctx) = get_wrapper(__p);
        return;
    }

    auto __p = __type.type->space->find(ctx->rval);
    if (!__p) /* Not found in array space. */
        throw error(
            "Member \""
            + ctx->rval
            + "\" not found in type \""
            +  __type.name() + '\"',ctx
         );

    /* Variables are assignable. */
    static_cast <wrapper &> (*ctx) = get_wrapper(__p);
    ctx->flag = true;
}


void ASTvisitor::visitConstructExpr(construct_expr *ctx) {
    ctx->space = top;
    if(ctx->type.name() == "void")
        throw error("Arrays cannot be void type!");
    for(auto __p : ctx->expr) {
        visit(__p);
        if(!__p->check("int",0))
            throw dark::error("Non-integer subscript!",ctx);
    }

    static_cast <wrapper &> (*ctx) = ctx->type;
}


void ASTvisitor::visitBinaryExpr(binary_expr *ctx) {
    ctx->space = top;
    visit(ctx->lval);
    visit(ctx->rval);
    const wrapper &__ltype = *ctx->lval;
    const wrapper &__rtype = *ctx->rval;

    /* Function and void type can't be operating..... */
    if(__ltype.type->is_function() || __rtype.type->is_function())
        throw error(
            std::string("No such operator \"") 
            + ctx->op.str + "\" for function type.",ctx
        );

    /* Special void case */
    if(__ltype.name() == "void" || __rtype.name() == "void")
        throw error(
            std::string("No such operator \"")
            + ctx->op.str
            + "\" for type \"void\".",ctx
        );

    /* Special case for operator = */
    if(ctx->op[0] == '=' && !ctx->op[1]) {
        if(!__ltype.assignable()) {
            throw error("LHS expression not assignable",ctx);
        } else if(!is_convertible(__rtype,__ltype)) {
            throw error(
                "Cannot convert type \"" 
                + __rtype.data()
                + "\" to \""
                + __ltype.data()
                + "\".",ctx
            );
        }
        /* Update the real type. */
        return void(static_cast <wrapper> (*ctx) = __ltype);
    }

    /* Non-assignment case.*/
    if(__ltype == __rtype) {
        if(__ltype.dimension())
            throw error(
                std::string("No such operator \"") 
                + ctx->op.str
                + "\" for array type.",ctx
            );

        if(__ltype.name() == "int") {
            // Only && || is forbidden
            static_cast <wrapper &> (*ctx) =
                get_wrapper(ASTTypeScanner::assert_int(ctx));
        } else if(__ltype.name() == "bool") {
            ASTTypeScanner::assert_bool(ctx);
            static_cast <wrapper> (*ctx) = __ltype;
            ctx->flag = false;
        } else if(__ltype.name() == "string") {
            static_cast <wrapper &> (*ctx) =
                get_wrapper(ASTTypeScanner::assert_string(ctx));
        } else if(__ltype.name() == "null") {
            static_cast <wrapper &> (*ctx) =
                get_wrapper(ASTTypeScanner::assert_null(ctx));
        } else {
            static_cast <wrapper &> (*ctx) =
                get_wrapper(ASTTypeScanner::assert_class(ctx));
        }
    } else { /* Comparison for different types. */
        /* Reference comparison: Only == and != . */
        if((ctx->op[0] == '!' || ctx->op[1] == '=')
        && (is_convertible(__ltype,__rtype) 
        ||  is_convertible(__rtype,__ltype))) {
            static_cast <wrapper> (*ctx) = get_wrapper("bool");
        } else {
            throw error(
                std::string("No such operator \"") 
                + ctx->op.str
                + "\" for type \""
                + __ltype.data()
                + "\" and \""
                + __rtype.data()
                + "\".",ctx
            );
        }
    }
}


void ASTvisitor::visitConditionExpr(condition_expr *ctx) {
    ctx->space = top;
    visit(ctx->cond);
    if(!ctx->cond->check("bool",0))
        throw error("Non bool type condition!",ctx->cond);

    visit(ctx->lval);
    visit(ctx->rval);
    const wrapper &__ltype = *ctx->lval;
    const wrapper &__rtype = *ctx->rval;

    if(__ltype != __rtype) {
        if(is_convertible(__ltype,__rtype)) {
            static_cast <wrapper &> (*ctx) = __rtype;
        } else if(is_convertible(__rtype,__ltype)) {
            static_cast <wrapper &> (*ctx) = __ltype;
        } else {
            /* Different types! */
            throw error("Different types in conditional expression!",ctx);
        }
        /* With implicit convertion , it is no longer assignable. */ 
        ctx->flag = false;
    } else {
        /* Assignable only when both are assignable. */
        static_cast <wrapper &> (*ctx) = __ltype;
        ctx->flag &= __rtype.flag;
    }
}


void ASTvisitor::visitAtomExpr(atom_expr *ctx) {
    ctx->space = top;
    auto *__p = top->find(ctx->name);
    ctx->real = __p;
    if (! __p) throw error("Such identifier doesn't exist: " + ctx->name);
    static_cast <wrapper &> (*ctx) = get_wrapper(__p);
}


void ASTvisitor::visitLiteralConstant(literal_constant *ctx) {
    ctx->space = top;
    switch(ctx->type) {
        case literal_constant::NUMBER:
            static_cast <wrapper &> (*ctx) = get_wrapper("int");
            break;
        case literal_constant::CSTRING:
            static_cast <wrapper &> (*ctx) = get_wrapper("string");
            break;
        case literal_constant::NULL_:
            /* This is a special case. The type will be created when used. */
            static_cast <wrapper &> (*ctx) = get_wrapper("null");
            break;
        case literal_constant::TRUE:
            static_cast <wrapper &> (*ctx) = get_wrapper("bool");
            break;
        case literal_constant::FALSE:
            static_cast <wrapper &> (*ctx) = get_wrapper("bool");
            break;
        default:
            throw error("How can this fucking happen!",ctx);
    }
}


void ASTvisitor::visitForStmt(for_stmt *ctx) {
    top = ctx->space = new scope {top};

    if(ctx->init) visit(ctx->init);
    if(ctx->cond) {
        visit(ctx->cond);
        if(!ctx->cond->check("bool",0))
            throw error("Non bool type condition!",ctx->cond);
    }
    if(ctx->step) visit(ctx->step);

    loop.push_back(ctx);
    top = ctx->space;
    visit(ctx->stmt);
    loop.pop_back();
}


void ASTvisitor::visitFlowStmt(flow_stmt *ctx) {
    ctx->space = top;
    if(ctx->flow[0] == 'r') {
        if(func.empty())
            throw error("Invalid flow statement: \"return \" outside function.",ctx);

        if(ctx->expr) {
            visit(ctx->expr);

            /* Check the return type. */
            if(!is_convertible(*ctx->expr,func.back()->type))
                throw error("Invalid flow return type: "
                "Cannot convert type \"" +
                    ctx->expr->data() +
                    "\" to \"" +
                    func.back()->type.data() +
                    "\".",ctx);
        } else {
            if(!func.back()->type.check("void",0))
                throw error("Invalid flow return type: return void in non-void function");
        }
        ctx->func = func.back();
    } else {
        if(loop.empty())
            throw error(
                std::string("Invalid flow statement: ")
                + (ctx->flow[0] == 'b' ? "\"break\"" : "\"continue\"" )
                + " outside loop."
            );
        ctx->loop = loop.back();
    }
}


/* While case : just check the statement. */
void ASTvisitor::visitWhileStmt(while_stmt *ctx) {
    ctx->space = new scope {.prev = top};
    visit(ctx->cond);
    if(!ctx->cond->check("bool",0))
        throw error("Non bool type condition!",ctx->cond);

    loop.push_back(ctx);
    top = ctx->space;
    visit(ctx->stmt);
    loop.pop_back();
}


/* Block case : visit all the statements. */
void ASTvisitor::visitBlockStmt(block_stmt *ctx) {
    ctx->space = top; /* Caution : might enter new scope! */
    for(auto __p : ctx->stmt) {
        if(dynamic_cast <block_stmt *> (__p)) {
            top = new scope {.prev = ctx->space};
        } else {
            top = ctx->space;
        }
        
        visit(__p);
    }
}


/* Check all the conditions. */
void ASTvisitor::visitBranchStmt(branch_stmt *ctx) {
    ctx->space = top;
    for(auto [__cond,__stmt] : ctx->data) {
        if(__cond) {
            visit(__cond);
            if(!__cond->check("bool",0))
                throw error("Non bool type condition!",__cond);
        }

        // Enter a new statement.
        top = new scope {.prev = ctx->space};
        visit(__stmt);
    }
}


/* Simple case : visit all the sub-expressions. */
void ASTvisitor::visitSimpleStmt(simple_stmt *ctx) {
    ctx->space = top;
    for(auto __p : ctx->expr) { visit(__p); }
}


/* Variable definition won't go into any new scope. */
void ASTvisitor::visitVariable(variable_def *ctx) {
    ctx->space = top;

    if(ctx->type.name() == "void")
        throw error("Variables cannot be void type!",ctx);

    for(auto &&[__name,__init] : ctx->init) {
        if(__init) {
            visit(__init);
            if(!is_convertible(*__init,ctx->type))
                throw error(
                    "Cannot convert type \"" +
                    __init->data() +
                    "\" to \"" +
                    ctx->type.data() +
                    "\".",ctx
                );
        }

        auto *__var = new variable;
        __var->name = __name;
        __var->type = ctx->type;
        __var->type.flag = true;
        auto __func = func.size() ? func[0] : nullptr;
        __var->unique_name = get_unique_name(__var,__func);
        if(__func) { /* Local variable. */
            __func->unique_mapping.push_back(__var);
        } else if(__init) { /* Global variable to initialize. */
            auto *__stmt = new simple_stmt;
            auto *__expr = new binary_expr;
            auto *__atom = new atom_expr;
            __atom->name = __name;
            __atom->real = __var;
            __expr->lval = __atom;
            __expr->rval = __init;
            __expr->op   = "=";
            __expr->space = global_init->space;
            static_cast <wrapper &> (*__expr) = __var->type;
            __stmt->expr.push_back(__expr);
            global_init->body->stmt.push_back(__stmt);
        }

        if(!top->insert(__name,__var))
            throw error("Duplicated variable name: \"" + __name + '\"',ctx);
    }
}


/* Special case : function's scope has been pre-declared. */
void ASTvisitor::visitFunction(function_def *ctx) {
    func.push_back(ctx);
    for(auto && __p : ctx->args) {
        auto *__var = new variable;
        static_cast <argument &> (*__var) = __p;
        __var->type.flag = true;
        __var->unique_name = get_unique_name(__var,func.front());
        if(!ctx->space->insert(__p.name,__var))
            throw error("Duplicated function argument name: \"" + __p.name + '\"',ctx);
    }
    top = ctx->space;
    visit(ctx->body);
    func.pop_back();
}


/* Special case : class's scope has been pre-declared. */
void ASTvisitor::visitClass(class_def *ctx) {
    for(auto __p : ctx->member) {
        auto __func = dynamic_cast <function_def *> (__p);
        if(!__func) continue; /* Member variable definition has been done! */
        visit(__func);
    }
}


}