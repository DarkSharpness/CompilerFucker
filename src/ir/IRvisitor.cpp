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
    auto *__var = variable_map[dynamic_cast <AST::variable *> (ctx->real)];

    /* Global or local variable. */
    if(__var->name[0] == '@' || __var->name[0] == '%') {
        result = __var;
    } else { /* Class member. */
        auto *__get = new get_stmt;
        __get->dst  = top->create_temporary(__var->type);
        __get->src  = __var;
        auto *__class = get_class(ctx->type->name);
        __get->idx  = class_map[ctx->type->name].index(ctx->name);
        __get->type = &class_map["ptr"];
        top->emplace_new(__get);
        result = __get->dst;
    }
}


void IRbuilder::visitLiteralConstant(AST::literal_constant *) {

}


void IRbuilder::visitForStmt(AST::for_stmt *) {}
void IRbuilder::visitFlowStmt(AST::flow_stmt *) {}
void IRbuilder::visitWhileStmt(AST::while_stmt *) {}
void IRbuilder::visitBlockStmt(AST::block_stmt *) {}
void IRbuilder::visitBranchStmt(AST::branch_stmt *) {}
void IRbuilder::visitSimpleStmt(AST::simple_stmt *) {}


/* This must be local variables. */
void IRbuilder::visitVariable(AST::variable_def *ctx) {

}


/* It will go down to member function and member variables. */
void IRbuilder::createGlobalClass(AST::class_def *ctx) {
    auto *__class = &class_map[ctx->name];
    for(auto __p : ctx->member) {
        auto *__func = dynamic_cast <AST::function_def *> (__p);
        if (__func) { createGlobalFunction(__func); continue; }

        auto *__var = dynamic_cast <AST::variable_def *> (__p);
        auto __info = get_type(__var->type);

        for(auto &&[__name,__init] : __var->init)
            __class->layout.push_back(__info),
            __class->member.push_back(__name);
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
}


}