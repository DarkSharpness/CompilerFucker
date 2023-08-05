#include "IRvisitor.h"


namespace dark::IR {

void IRbuilder::visitBracketExpr(AST::bracket_expr *ctx) {
    return visit(ctx->expr);
}

void IRbuilder::visitSubscriptExpr(AST::subscript_expr *ctx) {
    auto *__lhs = ctx->expr[0];
    visit(__lhs); /* Visit it first. */
    size_t __dim = __lhs->dimension();
    for(auto *__p : ctx->expr) {
        if(__p == __lhs) continue; /* Skip the first element. */
        auto *__get = new get_stmt;
        __get->src  = result;
        visit(__p);

        auto __type = get_type(__p->type);
        __get->idx  = get_stmt::NPOS;
        __get->dst  = top->create_temporary(__type);
        __get->type = &class_map[--__dim ? "ptr" : type_map[__type]];

        top->emplace_new(__get);
        result = __get->dst;
    }
}


void IRbuilder::visitFunctionExpr(AST::function_expr *ctx) {
    visit(ctx->body);

    auto *__call = new call_stmt;
    /* Function wrapper. */
    auto *__func = ctx->body->type->func;
    __call->func = function_map[__func];

    /* Member function case: Pass this pointer. */
    if(__func->unique_name.find(':'))
        __call->args.push_back(result);

    for(auto *__p : ctx->args) {
        visit(__p);
        __call->args.push_back(result);
    }

    __call->dest = top->create_temporary(get_type(ctx->type));
    top->emplace_new(__call);
    result = __call->dest;
}


void IRbuilder::visitUnaryExpr(AST::unary_expr *ctx) {

}


void IRbuilder::visitMemberExpr(AST::member_expr *ctx) {

}


void IRbuilder::visitConstructExpr(AST::construct_expr *) {}
void IRbuilder::visitBinaryExpr(AST::binary_expr *) {}
void IRbuilder::visitConditionExpr(AST::condition_expr *) {}


void IRbuilder::visitAtomExpr(AST::atom_expr *ctx) {
    /* If function, nothing should be done. */
    if(ctx->type->is_function()) return;

    /* A previously declared variable. */
    auto *__var = variable_map[ctx->real];

    /* Global or local variable. */
    if(__var->name[0] == '@' || __var->name[0] == '%') {
        result = __var;
    } else { /* Class member access. */
        auto *__get = new get_stmt;
        __get->src  = variable_map[ctx->space->find("this")];
        __get->dst  = top->create_temporary(__var->type,ctx->name);
        __get->idx  = get_class(ctx->type->name)->index(ctx->name);
        __get->type = &class_map["ptr"];

        top->emplace_new(__get);
        result = __get->dst;
    }
}


void IRbuilder::visitLiteralConstant(AST::literal_constant *ctx) {
    switch(ctx->type) {
        case AST::literal_constant::NUMBER: {
            auto *__int  = new integer_constant;
            __int->value = std::stoi(ctx->name);
            result = __int;
            break;
        }
        case AST::literal_constant::CSTRING: {
            result = createString(ctx->name);
            break;
        }
        case AST::literal_constant::NULL_ : {
            auto *__null__ = new null_constant;
            result = __null__;
            break;
        }
        default: {
            auto *__bool  = new boolean_constant;
            __bool->value = (ctx->type == AST::literal_constant::TRUE);
            result = __bool;
        }
    }
}


/* If empty , condition/step will be optimized out. */
void IRbuilder::visitForStmt(AST::for_stmt *ctx) {
    /**
     * @brief Structure as below:
     * __cond:
     * ...
     * <br __body __end>
     * __step:
     * ...
     * <jump __cond>
     * __body:
     * ...
     * <jump __step>
     * __end:
     * 
    */
    if(ctx->init) visit(ctx->init);

    std::string __name = top->create_for();
    jump_stmt  *__jump = nullptr ;
    block_stmt *__cond = nullptr;
    block_stmt *__body = new block_stmt;
    block_stmt *__step = nullptr;
    block_stmt *__end  = new block_stmt;
    __body->label = __name + "-body";
    __end ->label = __name + "-end";

    if(ctx->cond) {
        /* Jump to condition first. */
        __jump = new jump_stmt;
        __cond = new block_stmt;
        __cond->label = __name + "-cond";
        __jump->dest  = __cond;
        top->emplace_new(__jump);
        top->emplace_new(__cond);

        /* Build up condition body. */
        visit(ctx->cond);
        auto * __br = new branch_stmt;
        __br->cond  = result;
        __br->br[0] = __body;
        __br->br[1] = __end;
        /* Conditional jump to body. */
        top->emplace_new(__br);
    } else { /* Unconditional jump to body. */
        __jump = new jump_stmt;
        __cond = __body;
        __jump->dest = __cond;
        top->emplace_new(__jump);
    }

    if(ctx->step) {
        /* Build the step block first. */
        __step = new block_stmt;
        __step->label = __name + "-step";
        top->emplace_new(__step);
        visit(ctx->step);

        /* Unconditional jump to condition. */
        __jump = new jump_stmt;
        __jump->dest = __cond;
        top->emplace_new(__jump);
    } else { /* No need to jump to step. */
        __step = __cond;
    }

    /* Build up body. */
    top->emplace_new(__body);
    loop_flow.push_back({.bk = __end,.ct = __step});
    visit(ctx->stmt);
    loop_flow.pop_back();

    /* Unconditionally jump to step. */
    __jump = new jump_stmt;
    __jump->dest = __step;
    top->emplace_new(__jump);
    top->emplace_new(__end);
}


void IRbuilder::visitFlowStmt(AST::flow_stmt *ctx) {
    if(ctx->flow[0] == 'r') {
        auto *__ret = new return_stmt;
        if(ctx->expr) {
            visit(ctx->expr);
            __ret->rval = result;
        }
        top->emplace_new(__ret);
    } else {
        auto *__loop = loop_flow.back() [ctx->flow[0] == 'c'];
        auto *__jump = new jump_stmt;
        __jump->dest = __loop;
        top->emplace_new(__jump);
    }
}


void IRbuilder::visitWhileStmt(AST::while_stmt *ctx) {
    /**
     * @brief Structure as below:
     * __cond:
     * ...
     * <br __beg __end>
     * __beg:
     * ...
     * <jump __cond>
     * __end:
     * 
    */

    std::string __name = top->create_while();

    /* Jump into condition block. */
    auto *__cond = new block_stmt;
    __cond->label = __name + "-cond";
    auto *__jump = new jump_stmt;
    top->emplace_new(__jump);
    top->emplace_new(__cond);

    /* Building condition block and divide. */
    auto *__end  = new block_stmt;
    __end->label = __name + "-end";
    auto *__beg  = new block_stmt;
    __beg->label = __name + "-beg";
    visit(ctx->cond);
    auto * __br  = new branch_stmt;
    __br ->cond  = result;
    __br ->br[0] = __beg;
    __br ->br[1] = __end;
    top->emplace_new(__br);
    top->emplace_new(__br->br[0]);

    /* Building while body. */
    loop_flow.push_back({.bk = __end,.ct = __cond});
    visit(ctx->stmt);
    loop_flow.pop_back();

    /* End of body block. */
    __jump       = new jump_stmt;
    __jump->dest = __beg;
    top->emplace_new(__jump);
    top->emplace_new(__end);
}


void IRbuilder::visitBranchStmt(AST::branch_stmt *ctx) {
    /**
     * @brief Structure as below:
     * <br true-0 false-0>
     * true-0:
     * ...
     * <jump end>
     * false-0:
     * <br true-1 false-1>
     * true-1:
     * ...
     * <jump end>
     * false-1: (if no else_stmt ,then 'false' => 'end')
     * ...
     * <jump end>
     * end:
     * 
    */

    auto *__end  = new block_stmt;
    std::string __name = top->create_branch();
    __end->label = __name + "-end";
    size_t i = 0;

    for(auto &&[__cond,__stmt] : ctx->data) {
        block_stmt *__next = __end;
        if(__cond) {
            std::string __num = std::to_string(i++);
            visit(__cond);

            /* Opens a new branch! */
            auto * __br = new branch_stmt;
            __br->cond  = result;
            __br->br[0] = new block_stmt;
            __br->br[0]->label = string_join(__name,"-true-",__num);
            top->emplace_new(__br);
            top->emplace_new(__br->br[0]);

            /* If + else case. */
            if(i != ctx->data.size()) {
                __br->br[1] = new block_stmt;
                __br->br[1]->label = string_join(__name,"-false-",__num);
            } else { /* Last if (no else case). */
                __br->br[1] = __end;
            }

            __next = __br->br[1];
        }

        visit(__stmt);
        auto *__jump = new jump_stmt;
        __jump->dest = __end;
        top->emplace_new(__jump);
        top->emplace_new(__next);
    }
}


void IRbuilder::visitBlockStmt(AST::block_stmt *ctx) {
    for(auto __p : ctx->stmt)
        visit(__p);
}


void IRbuilder::visitSimpleStmt(AST::simple_stmt *ctx) {
    for(auto __p : ctx->expr)
        visit(__p);
}


/* This must be local variables. */
void IRbuilder::visitVariable(AST::variable_def *ctx) {
    if(!top) return visitGlobalVariable(ctx);
    for(auto [__name,__init] : ctx->init) {
        if(!__init) continue;
        auto *__var = variable_map[ctx->space->find(__name)];
        auto __load = new load_stmt;
        __load->src = __var;
        __load->dst = top->create_temporary(__var->type);
        top->emplace_new(__load);
        result = __load->dst;
    }
}


void IRbuilder::visitFunction(AST::function_def *ctx) {
    top = function_map[ctx];
    visit(ctx->body);
}


void IRbuilder::visitClass(AST::class_def *ctx) {
    for(auto __p : ctx->member) {
        top = nullptr;
        visit(__p);
    }
}


void IRbuilder::visitGlobalVariable(AST::variable_def *ctx) {
    /* This will do nothing. */
}


/* It will go down to member function and member variables. */
void IRbuilder::createGlobalClass(AST::class_def *ctx) {
    auto *__class = &class_map[ctx->name];
    for(auto __p : ctx->member) {
        auto *__func = dynamic_cast <AST::function_def *> (__p);
        if (__func) { createGlobalFunction(__func); continue; }

        auto *__var = dynamic_cast <AST::variable_def *> (__p);
        auto __info = get_type(__var->type);

        for(auto &&[__name,__init] : __var->init) {
            __class->layout.push_back(__info);
            __class->member.push_back(__name);
            auto *__p   = ctx->space->find(__name);
            auto *__var = new variable;
            __var->name = __p->unique_name;
            __var->type = get_type(__p->type);
            variable_map[__p] = __var;
        }
    }
}


/* This will be called globally to create a global variable with no initialization. */
void IRbuilder::createGlobalVariable(AST::variable *ctx) {
    auto *__var = new variable;
    __var->name = ctx->unique_name;
    __var->type = get_type(ctx->type);
    variable_map[ctx] = __var;
    literal *__lit;
    switch(__var->type) {
        case typeinfo::I1 : __lit = new boolean_constant;
        case typeinfo::I32: __lit = new integer_constant;
        default:            __lit = new null_constant;
    }
    global_variable.push_back({__var,__lit});
}


/* This will be called globally to create a global or member function. */
void IRbuilder::createGlobalFunction(AST::function *ctx) {
    global_function.emplace_back();
    top = &global_function.back();
    top->name = ctx->unique_name;
    top->type = get_type(ctx->type);
    top->emplace_new(new block_stmt);
    top->stmt[0]->label = "entry";

    /* All the local variables used. */
    for(auto *__p : ctx->unique_mapping) {
        auto *__var = new variable;
        __var->name = __p->unique_name;
        __var->type = get_type(__p->type);
        variable_map[__p] = __var;

        auto *__alloca = new allocate_stmt;
        __alloca->dest = __var;
        top->emplace_new(__alloca);
    }

    /* Member function case. */
    if(top->name[0] != ':') {
        /* Argument list : add argument %this%. */
        auto *__var = new variable;
        __var->name = "%this";
        __var->type = typeinfo::PTR;
        top->args.push_back(__var);
        auto __this = ctx->space->find("this");
        if (!__this) throw error("Invalid member function",ctx);
        variable_map[__this] = __var;

        /* Load %this% pointer first. */
        auto __load = new load_stmt;
        __load->dst = top->create_temporary(__var->type);
        __load->src = __var;
        top->emplace_new(__load);
    }

    /* Add all of the function params. */
    for(auto &&[__name,__type] : ctx->args) {
        auto *__p   = ctx->space->find(__name);
        auto *__var = new variable;
        __var->name = __p->unique_name;
        __var->type = get_type(__p->type);
        variable_map[__p] = __var;
        top->args.push_back(__var);
    }

    function_map[ctx] = top;
}


}