#include "ASTvisitor.h"


namespace dark::AST {


void ASTvisitor::visitBracketExpr(bracket_expr *ctx) {
    ctx->space = top;
    return visit(ctx->expr);
}

void ASTvisitor::visitSubscriptExpr(subscript_expr *ctx) {
    ctx->space = top;
    for(size_t i = 1 ; i < ctx->expr.size() ; ++i) {
        auto __p = ctx->expr[i];
        visit(__p);
        if(!current_type.check("int",0))
            throw error("Non-integer subscript!",ctx);
    }
    visit(ctx->expr.front());
    current_type.info -= ctx->expr.size() - 1;
    current_type.flag = true; /* Array access are re-assignable. */
    if(current_type.info < 0)
        throw error("Too many subscripts!",ctx);
}

void ASTvisitor::visitFunctionExpr(function_expr *ctx) {
    ctx->space = top;
    visit(ctx->body);
    if(!current_type.type->is_function())
        throw error("Unknown function: \"" + current_type.name() + '\"',ctx->body);
    auto *__func = current_type.type->func;
    if(__func->args.size() != ctx->args.size())
        throw error("Function params length dismatch!",ctx);

    for(size_t i = 0 ; i < ctx->args.size() ; ++i) {
        auto __p = ctx->args[i];
        visit(__p);
        if(!is_convertible(current_type,__func->args[i].type))
            throw error(
                "Function params dismatch: "
                "Cannot convert type \"" 
                + current_type.data()
                + "\" to \""
                + __func->args[i].type.data()
                + "\".",ctx
            );
    }
    current_type = __func->type;
}


void ASTvisitor::visitUnaryExpr(unary_expr *ctx) {
    ctx->space = top;
    visit(ctx->expr);
    if(ctx->op[1]) {
        if(!current_type.check("int",0)) {
            throw error(
                std::string("No such operator ")
                + ctx->op.str
                + " for type \""
                + current_type.name()
                + "\".",ctx
            );
        }
        if(!current_type.assignable())
            throw error("No self-iteration for right value type.",ctx);

        current_type.flag = !ctx->op[2];
    } else {
        if(((ctx->op[0] == '+' || ctx->op[0] == '-') && !current_type.check("int",0))
        || ((ctx->op[0] == '!' || ctx->op[0] == '~') && !current_type.check("bool",0))) {
            throw error(
                std::string("No such operator ")
                + ctx->op.str
                + " for type \""
                + current_type.name()
                + "\".",ctx
            );
        }

        /* Of course not assignable. */
        current_type.flag = false;
    }
}


void ASTvisitor::visitMemberExpr(member_expr *ctx) {
    ctx->space = top;
    visit(ctx->lval);

    if(current_type.type->is_function())
        throw error("Function doesn't possess scope!",ctx);
    if(current_type.dimension()) {
        auto __p = class_map["__array__"]->space->find(ctx->rval);
        if (!__p) /* Not found in array space. */
            throw error( "Member \"" + ctx->rval + "\" not found in array type",ctx);
        current_type = get_wrapper(__p);
        return;
    }

    auto __p = current_type.type->space->find(ctx->rval);
    if (!__p) /* Not found in array space. */
        throw error(
            "Member \""
            + ctx->rval
            + "\" not found in type \""
            +  current_type.name() + '\"',ctx
         );

    current_type = get_wrapper(__p);
    current_type.flag = true;  // Variables are assignable.
}


void ASTvisitor::visitConstructExpr(construct_expr *ctx) {
    ctx->space = top;
    if(ctx->type.name() == "void")
        throw error("Arrays cannot be void type!");
    for(auto __p : ctx->expr) {
        visit(__p);
        if(!current_type.check("int",0))
            throw dark::error("Non-integer subscript!",ctx);
    }
    current_type = ctx->type;
}


void ASTvisitor::visitBinaryExpr(binary_expr *ctx) {
    ctx->space = top;
    visit(ctx->lval);
    auto temp_type = current_type;

    visit(ctx->rval);

    /* Function and void type can't be operating..... */
    if(temp_type.type->is_function() || current_type.type->is_function())
        throw error(
            std::string("No such operator \"") 
            + ctx->op.str + "\" for function type.",ctx
        );

    /* Special void case */
    if(temp_type.name() == "void" || current_type.name() == "void")
        throw error(
            std::string("No such operator \"")
            + ctx->op.str
            + "\" for type \"void\".",ctx
        );

    /* Special case for operator = */
    if(ctx->op[0] == '=' && !ctx->op[1]) {
        if(!temp_type.assignable()) {
            throw error("LHS expression not assignable",ctx);
        } else if(!is_convertible(current_type,temp_type)) {
            throw error(
                "Cannot convert type \"" 
                + current_type.data()
                + "\" to \""
                + temp_type.data()
                + "\".",ctx
            );
        }
        /* Update the real type. */
        return void(current_type = temp_type);
    }

    /* Non assignment case.*/
    if(temp_type == current_type) {
        if(temp_type.dimension())
            throw error(
                std::string("No such operator \"") 
                + ctx->op.str
                + "\" for array type.",ctx
            );

        if(temp_type.name() == "int") {
            // Only && || is forbidden
            if(ctx->op[0] == ctx->op[1]
            && ctx->op[0] == '&' || ctx->op[0] == '|') {
                throw error(
                    std::string("No such operator \"")
                    + ctx->op.str
                    + "\" for type \"int\".",ctx
                );
            }
            switch(ctx->op[0]) {
                case '<':
                case '>':
                    if(ctx->op[1] == ctx->op[0]) break;
                case '!':
                case '=':
                    /* Comparison operators. */
                    current_type = get_wrapper("bool");
            }
        } else if(temp_type.name() == "bool") {
            switch(ctx->op[0]) {
                case '&':
                case '|':
                    if(ctx->op[1]) break;
                case '>':
                case '<':
                case '+':
                case '-':
                case '*':
                case '/':
                case '%':
                case '^':
                    throw error(
                        std::string("No such operator ") 
                        + ctx->op.str
                        + " for type \"bool\".",ctx
                    );
            }
        } else if(temp_type.name() == "string") {
            switch(ctx->op[0]) {
                case '>':
                case '<':
                    if(ctx->op[0] != ctx->op[1])
                        break;
                case '&':
                case '|':
                case '-':
                case '*':
                case '/':
                case '%':
                case '^':
                    throw error(
                        std::string("No such operator ") 
                        + ctx->op.str
                        + " for type \"string\".",ctx
                    );
            }
            switch(ctx->op[0]) {
                case '+': break; /* No need to change */
                default : current_type = get_wrapper("bool");
            }
        } else if(temp_type.name() == "null") {
            throw error(
                std::string("No such operator ") 
                + ctx->op.str
                + " for \"null\".",ctx
            );
        } else {
            /* No operator other than != and and for them. */
            if(ctx->op[0] != '=' && ctx->op[0] != '!')
                throw error(
                    std::string("No such operator ") 
                    + ctx->op.str
                    + " for type \""
                    + temp_type.name()
                    + "\".",ctx
                );
            current_type = get_wrapper("bool");
        }
    } else {
        /* Reference comparison: Only == and != . */
        if((ctx->op[0] == '!' || ctx->op[1] == '=')
        && (is_convertible(temp_type,current_type) 
        ||  is_convertible(current_type,temp_type))) {
            current_type = get_wrapper("bool");            
        } else {
            throw error(
                std::string("No such operator \"") 
                + ctx->op.str
                + "\" for type \""
                + temp_type.data()
                + "\" and \""
                + current_type.data()
                + "\".",ctx
            );
        }
    }
    /* No longer assignable. */
    current_type.flag = false;
}

void ASTvisitor::visitConditionExpr(condition_expr *ctx) {
    ctx->space = top;
    visit(ctx->cond);
    if(!current_type.check("bool",0))
        throw error("Non bool type condition!",ctx->cond);

    visit(ctx->lval);
    auto temp_type = current_type;
    visit(ctx->rval);

    if(current_type != temp_type) {
        if(is_convertible(current_type,temp_type)) {
            current_type = temp_type;
        } else if(is_convertible(temp_type,current_type)) {
            // Do nothing here.
        } else {
            // Different type error!
            throw error("Different types in conditional expression!",ctx);
        }
        // With implicit convertion , it is no longer assignable.
        current_type.flag = false;
    } else {
        // If assignable, both are assignable.
        current_type.flag &= temp_type.flag;
    }
}


void ASTvisitor::visitAtomExpr(atom_expr *ctx) {
    ctx->space = top;
    auto *__p = top->find(ctx->name);
    ctx->real = __p;
    if (! __p) throw error("Such identifier doesn't exist: " + ctx->name);
    current_type = get_wrapper(__p);
}


void ASTvisitor::visitLiteralConstant(literal_constant *ctx) {
    ctx->space = top;
    switch(ctx->type) {
        case literal_constant::NUMBER:
            current_type = get_wrapper("int");
            break;
        case literal_constant::CSTRING:
            current_type = get_wrapper("string");
            break;
        case literal_constant::NULL_:
            /* This is a special case. The type will be created when used. */
            current_type = get_wrapper("null");
            break;
        case literal_constant::TRUE:
            current_type = get_wrapper("bool");
            break;
        case literal_constant::FALSE:
            current_type = get_wrapper("bool");
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
        if(!current_type.check("bool",0))
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
            if(!is_convertible(current_type,func.back()->type))
                throw error("Invalid flow return type: "
                "Cannot convert type \"" +
                    current_type.data() +
                    "\" to \"" +
                    func.back()->type.data() +
                    "\".",ctx);
        } else {
            if(!func.back()->type.check("void",0))
                throw error("Invalid flow return type: return void in non-void function");
        }
    } else {
        if(loop.empty())
            throw error(
                std::string("Invalid flow statement: ")
                + (ctx->flow[0] == 'b' ? "\"break\"" : "\"continue\"" )
                + " outside loop."
            );
    }
}


/* While case : just check the statement. */
void ASTvisitor::visitWhileStmt(while_stmt *ctx) {
    ctx->space = new scope {.prev = top};
    visit(ctx->cond);
    if(!current_type.check("bool",0))
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
            if(!current_type.check("bool",0))
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
    auto *__def  = static_cast <definition *> (ctx);
    __def->space = top;

    if(ctx->type.name() == "void")
        throw error("Variables cannot be void type!",__def);

    for(auto &&[__name,__init] : ctx->init) {
        if(__init) {
            visit(__init);
            if(!is_convertible(current_type,ctx->type))
                throw error(
                    "Cannot convert type \"" +
                    current_type.data() +
                    "\" to \"" +
                    ctx->type.data() +
                    "\".",__def
                );
        }
        auto *__var = new variable;
        __var->name = __name;
        __var->type = ctx->type;
        __var->type.flag = true;
        __var->unique_name = get_unique_name(__var->name,func.size() ? func[0] : nullptr);
        if(!top->insert(__name,__var))
            throw error("Duplicated variable name: \"" + __name + '\"',__def);
    }
}


/* Special case : function's scope has been pre-declared. */
void ASTvisitor::visitFunction(function_def *ctx) {
    func.push_back(ctx);
    for(auto && __p : ctx->args) {
        auto *__var = new variable;
        static_cast <argument &> (*__var) = __p;
        __var->type.flag = true;
        __var->unique_name = get_unique_name(__var->name,func.front());
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
        if(!__func) continue; // Member variable definition has been done!
        visit(__func);
    }
}


}