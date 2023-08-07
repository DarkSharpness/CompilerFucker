#include "IRbuilder.h"


namespace dark::IR {

void IRbuilder::visitBracketExpr(AST::bracket_expr *ctx) {
    return visit(ctx->expr);
}

void IRbuilder::visitSubscriptExpr(AST::subscript_expr *ctx) {
    auto *__lhs = ctx->expr[0];
    visit(__lhs); /* Visit it first. */
    auto __type = result->get_value_type();
    for(auto __iter = ctx->expr.begin() + 1,__end = ctx->expr.end();;) {
        auto *__p   = *__iter;
        auto *__get = new get_stmt;
        __get->src  = result;
        visit(__p);
        __get->idx  = result;
        __get->mem  = get_stmt::NPOS;
        __get->dst  = top->create_temporary(__type,"arrayidx");
        --__type.dimension; /* Decrease the dimension by one. */

        top->emplace_new(__get);

        if(++__iter == __end) {
            result = __get->dst;
            break;
        }

        result = __get->dst;
        result = safe_load_variable();
    }

    if(!ctx->assignable()) result = safe_load_variable();
}


void IRbuilder::visitFunctionExpr(AST::function_expr *ctx) {
    visit(ctx->body);

    auto *__call = new call_stmt;
    /* Function wrapper. */
    auto *__func = ctx->body->type->func;
    __call->func = function_map[__func];

    /* Member function case: Pass 'this' pointer. */
    if(__func->unique_name[0] != ':')
        __call->args.push_back(result);
 
    for(auto *__p : ctx->args) {
        visit(__p);
        __call->args.push_back(result);
    }

    __call->dest = top->create_temporary(get_type(*ctx),"call");
    top->emplace_new(__call);
    result = __call->dest;
}


void IRbuilder::visitUnaryExpr(AST::unary_expr *ctx) {
    visit(ctx->expr);
    if(ctx->op[1]) { /* ++ or -- */
        /* Load into memory from a variable. */
        auto *__tmp = safe_load_variable();

        auto *__bin = new binary_stmt;
        __bin->op   = ctx->op[1] == '+' ? binary_stmt::ADD : binary_stmt::SUB;
        __bin->dest = top->create_temporary(
            wrapper {&__integer_class__,0},
            binary_stmt::str[__bin->op]
        );
        __bin->lvar = __tmp;
        __bin->rvar = __one1__;
        top->emplace_new(__bin);

        auto *__store = new store_stmt;
        __store->dst  = safe_cast <non_literal *> (result);
        __store->src  = __bin->dest;
        top->emplace_new(__store);

        /* If op[2](suffix case), take loaded value. Else take operated value.  */
        if(!ctx->assignable()) result = ctx->op[2] ? __tmp : __bin->dest;
    } else {
        if(ctx->op[0] == '+') return;
        auto *__bin = new binary_stmt;
        __bin->op   = ctx->op[0] == '-' ? binary_stmt::SUB : binary_stmt::XOR;
        __bin->dest = top->create_temporary(
            wrapper { ctx->op[0] == '!' ? 
                (dark::IR::typeinfo *)&__boolean_class__ :
                (dark::IR::typeinfo *)&__integer_class__,0
            } ,
            binary_stmt::str[__bin->op]
        );
        __bin->lvar = ctx->op[0] == '-' ? (definition *)__zero__ : 
                      ctx->op[0] == '!' ? (definition *)__true__ : __one1__;
        __bin->rvar = result;
        top->emplace_new(__bin);
        result = __bin->dest;
    }
}



void IRbuilder::visitMemberExpr(AST::member_expr *ctx) {
    visit(ctx->lval);

    /* Member function case. */
    if(ctx->type->is_function()) return;

    /* Class member access. */
    auto *__class = get_class(ctx->lval->type->name);
    auto *__get = new get_stmt;
    __get->src  = result;
    __get->dst  = top->create_temporary(++get_type(*ctx),
        string_join(ctx->lval->type->name,".",ctx->rval,'-'));
    __get->idx  = __zero__;
    __get->mem  = __class->index(ctx->rval);

    top->emplace_new(__get);
    result = __get->dst;

    /* If not assignable , load it immediately! */
    if(!ctx->assignable()) result = safe_load_variable();
}


void IRbuilder::visitConstructExpr(AST::construct_expr *) {
    throw error("Not implemented!");
}


void IRbuilder::visitBinaryExpr(AST::binary_expr *ctx) {
    /* Assignment case. */
    if(ctx->op[0] == '=' && !ctx->op[1]) {
        visit(ctx->rval);
        auto *__store = new store_stmt;
        __store->src  = result;

        visit(ctx->lval);
        __store->dst  = safe_cast <non_literal *> (result);

        top->emplace_new(__store);
        if(!ctx->assignable()) result = safe_load_variable();
        return;
    }

    /* Normal arithmetic case. */
    if(ctx->op == "+" || ctx->op == "-" || ctx->op == "|"
    || ctx->op == "&" || ctx->op == "^" || ctx->op == "<<"
    || ctx->op == ">>"|| ctx->op == "*" || ctx->op == "/" || ctx->op == "%") {
        auto *__bin = new binary_stmt;
        switch(ctx->op[0]) {
            case '+': __bin->op = binary_stmt::ADD; break;
            case '-': __bin->op = binary_stmt::SUB; break;
            case '|': __bin->op = binary_stmt::OR;  break;
            case '&': __bin->op = binary_stmt::AND; break;
            case '^': __bin->op = binary_stmt::XOR; break;
            case '<': __bin->op = binary_stmt::SHL; break;
            case '>': __bin->op = binary_stmt::ASHR;break;
            case '*': __bin->op = binary_stmt::MUL; break;
            case '/': __bin->op = binary_stmt::SDIV;break;
            case '%': __bin->op = binary_stmt::SREM;break;
        }
        visit(ctx->lval);
        __bin->lvar = result;
        visit(ctx->rval);
        __bin->rvar = result;
        __bin->dest = top->create_temporary({&__integer_class__,0},binary_stmt::str[__bin->op]);
        __bin->dest->type = get_type(*ctx);

        top->emplace_new(__bin);
        result = __bin->dest;
        return;
    }

    /* Noraml logical case. */
    if(ctx->op[0] != '&' && ctx->op[0] != '|') {
        auto *__cmp = new compare_stmt;
        __cmp->op = ctx->op == "!=" ? compare_stmt::NE :
                    ctx->op == "==" ? compare_stmt::EQ :
                    ctx->op == "<"  ? compare_stmt::LT :
                    ctx->op == ">"  ? compare_stmt::GT :
                    ctx->op == "<=" ? compare_stmt::LE :
                    ctx->op == ">=" ? compare_stmt::GE :
                        throw error("Unknown operator!");
        visit(ctx->lval);
        __cmp->lvar = result;
        visit(ctx->rval);
        __cmp->rvar = result;
        __cmp->dest = top->create_temporary(
            {&__boolean_class__,0},
            compare_stmt::str[__cmp->op]
        );

        top->emplace_new(__cmp);
        result = __cmp->dest;
        return;
    }

    /* Short-circuit logic case. */
    std::string __name = top->create_branch();
    bool __is_or = ctx->op[0] == '|';

    visit(ctx->lval);
    auto *__br  = new branch_stmt;
    auto *__end = new block_stmt;
    auto *__new = new block_stmt;
    __br->cond  = result;
    __br->br[ __is_or] = __new;
    __br->br[!__is_or] = __end;
    __br->br[ __is_or]->label = __name + '-' + (__is_or ? "false" : "true");
    __br->br[!__is_or]->label = __name + "-end";

    /* Name of previous branch */
    auto *__block = top->stmt.back();
    top->emplace_new(__br);
    top->emplace_new(__new);

    visit(ctx->rval);

    auto *__jump = new jump_stmt;
    __jump->dest = __end;

    top->emplace_new(__jump);
    top->emplace_new(__end);

    auto *__phi  = new phi_stmt;
    __phi->cond.push_back({result,__new});
    __phi->cond.push_back({__is_or ? __true__ : __false__,__block});
    __phi->dest = top->create_temporary({&__boolean_class__,0},"phi");
    top->emplace_new(__phi);

    result = __phi->dest;
}


void IRbuilder::visitConditionExpr(AST::condition_expr *ctx) {
    visit(ctx->cond);
    std::string __name = top->create_branch();
    jump_stmt  *__jump = nullptr;

    auto *__br  = new branch_stmt;
    __br->cond  = result;
    __br->br[0] = new block_stmt;
    __br->br[1] = new block_stmt;
    __br->br[0]->label = __name + "-true";
    __br->br[1]->label = __name + "-false";

    top->emplace_new(__br);
    top->emplace_new(__br->br[0]);
    visit(ctx->lval);

    auto *__phi  = new phi_stmt;
    auto *__end  = new block_stmt;
    __phi->cond.push_back({result,__br->br[0]});
    __jump       = new jump_stmt;
    __jump->dest = __end;

    top->emplace_new(__jump);
    top->emplace_new(__br->br[1]);
    visit(ctx->rval);

    __phi->cond.push_back({result,__br->br[1]});
    __jump       = new jump_stmt;
    __jump->dest = __end;
    __end->label = __name + "-end";
    /* If assignable, return the pointer to the data. */
    __phi ->dest = top->create_temporary(get_type(*ctx) + ctx->flag,"ternary");

    top->emplace_new(__jump);
    top->emplace_new(__end);
    top->emplace_new(__phi);

    result = __phi->dest;
}


void IRbuilder::visitAtomExpr(AST::atom_expr *ctx) {
    /* If function, nothing should be done. */
    if(ctx->type->is_function()) {
        auto __name = ctx->type->func->unique_name;
        /* Member function case. */
        if(__name[0] != ':' && __name != "main")
            result = get_this_pointer();
        return;
    }

    /* Special case : this pointer has been loaded! */

    auto __name = ctx->real->unique_name;
    /* Global or local variable. */
    if(__name[0] == '@' || __name[0] == '%') {
        result = variable_map[ctx->real];
    } else { /* Class member access. */
        size_t __len  = __name.find(':');
        auto *__class = get_class(__name.substr(0,__len));
        auto *__get = new get_stmt;
        __get->src  = get_this_pointer();
        __get->dst  = top->create_temporary(++get_type(*ctx),ctx->name);
        __get->idx  = __zero__;
        __get->mem  = __class->index(ctx->name);

        top->emplace_new(__get);
        result = __get->dst;
    }

    /* Not assignable, so take the value out. */
    if(!ctx->assignable()) result = safe_load_variable();
}


void IRbuilder::visitLiteralConstant(AST::literal_constant *ctx) {
    switch(ctx->type) {
        case AST::literal_constant::NUMBER:
            result = new integer_constant{std::stoi(ctx->name)};
            break;
        case AST::literal_constant::CSTRING:
            result = create_string(ctx->name);
            break;
        case AST::literal_constant::NULL_ :
            result = __null__;
            break;
        default:
            result = ctx->type == AST::literal_constant::TRUE ? __true__ : __false__;
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
        __cond = new block_stmt;
        __cond->label = __name + "-cond";
        __jump = new jump_stmt;
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
        __cond = __body;
        __jump = new jump_stmt;
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
        __ret->func = function_map[ctx->func];
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
    __jump->dest = __cond;
    top->emplace_new(__jump);
    top->emplace_new(__cond);

    /* Building condition block and divide. */
    visit(ctx->cond);
    auto *__end  = new block_stmt;
    __end->label = __name + "-end";
    auto *__beg  = new block_stmt;
    __beg->label = __name + "-beg";
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
    __jump->dest = __cond;
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
            visit(__cond);
            std::string __num = std::to_string(i++);

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
    for(auto __p : ctx->stmt) visit(__p);
}


void IRbuilder::visitSimpleStmt(AST::simple_stmt *ctx) {
    for(auto __p : ctx->expr) visit(__p);
}


/* This must be local variables. */
void IRbuilder::visitVariable(AST::variable_def *ctx) {
    if(!top) return; /* Global variable definition case. */
    for(auto [__name,__init] : ctx->init) {
        if(!__init) continue;
        visit(__init);
        auto *__store = new store_stmt;
        __store->src  = result;
        __store->dst  = variable_map[ctx->space->find(__name)];
        top->emplace_new(__store);
    }
}


void IRbuilder::visitFunction(AST::function_def *ctx) {
    top = function_map[ctx];
    visit(ctx->body);
    if(top->type.name() == "void") {
        auto *__ret = new return_stmt;
        __ret->func = top;
        top->emplace_new(__ret);
    }
}


void IRbuilder::visitClass(AST::class_def *ctx) {
    for(auto __p : ctx->member) {
        top = nullptr;
        visit(__p);
    }
}


void IRbuilder::visitGlobalVariable(AST::variable_def *ctx) {
    for(auto &&[__name,__init] : ctx->init)
        visitGlobalVariable(
            safe_cast <AST::variable *> (global_scope->find(__name)),
            dynamic_cast <AST::literal_constant *>(__init)
        );
}


/* It will go down to member function and member variables. */
void IRbuilder::visitGlobalClass(AST::class_def *ctx) {
    auto *&__type = class_map[ctx->name];
    if(!__type)  {
        __type = new class_type {"%struct." + ctx->name};
        for(auto __p : ctx->member)
            function_cnt += dynamic_cast <AST::function_def *> (__p) != nullptr;
        return;
    }

    class_type *__class = safe_cast <class_type *> (__type);
    for(auto __p : ctx->member) {
        if (auto *__func = dynamic_cast <AST::function_def *> (__p)) {
            visitGlobalFunction(__func);
        } else {
            auto *__var = safe_cast <AST::variable_def *> (__p);
            auto __info = get_type(__var->type);
            /* Make the member variables. */
            for(auto &&[__name,__init] : __var->init) {
                __class->layout.push_back(__info);
                __class->member.push_back(__name);
                auto *__p   = ctx->space->find(__name);
                auto *__var = new variable;
                __var->name = __p->unique_name;
                __var->type = ++__info;
                variable_map[__p] = __var;
            }
        }
    }
}


/* This will be called globally to create a global variable with no initialization. */
void IRbuilder::visitGlobalVariable(AST::variable *ctx,AST::literal_constant *lit) {
    /* Make the global variables. */
    auto *__var = new variable;
    __var->name = ctx->unique_name;
    __var->type = ++get_type(ctx->type);
    variable_map[ctx] = __var;

    literal *__lit; /* Literal constant. */
    auto __type = (--__var->type).name();
    if(__type == "i1") {
        __lit = new boolean_constant { lit && lit->name[0] == 't'};
    } else if(__type == "i32") {
        __lit = lit ? new integer_constant {std::stoi(lit->name)} : __zero__;
    } else if(!lit || lit->type == lit->NULL_) {
        __lit = __null__;
    } else {
        __lit = new pointer_constant {create_string(lit->name)};
    }

    global_variable.push_back({__var,__lit});
}


/* This will be called globally to create a global or member function. */
void IRbuilder::visitGlobalFunction(AST::function_def *ctx) {
    top = &global_function.emplace_back();
    function_map[ctx] = top;
    top->name = ctx->unique_name;
    top->type = get_type(ctx->type);
    top->emplace_new(new block_stmt);
    top->stmt[0]->label = "entry";

    /* All the local variables used. */
    for(auto *__p : ctx->unique_mapping) {
        auto *__var = new variable;
        __var->name = __p->unique_name;
        __var->type = ++get_type(__p->type);
        variable_map[__p] = __var;

        auto *__alloca = new allocate_stmt;
        __alloca->dest = __var;
        top->emplace_new(__alloca);
    }

    std::vector <store_stmt *> __store;
    __store.reserve(ctx->args.size() + 1);

    /* Global function case. */
    if(top->name[0] != ':' & top->name != "main") {
        auto __this = ctx->space->find("this");
        if (!__this) throw error("Invalid member function!",ctx);
        __store.push_back(visitFunctionParam(__this));

        /* May be this pointer should be pre-loaded. */
        // safe_load_variable();
    }

    /* Add all of the function params. */
    for(auto &&[__name,__type] : ctx->args)
        __store.push_back(visitFunctionParam(ctx->space->find(__name)));

    for(auto __s : __store) top->emplace_new(__s);
}


store_stmt *IRbuilder::visitFunctionParam(AST::identifier *__p) {
    auto *__var = new variable;
    __var->name = __p->unique_name;
    __var->type = get_type(__p->type);
    top->args.push_back(__var);

    auto *__store = new store_stmt;
    __store->src  = __var; /* Variable in the param */

    auto *__alloc = new allocate_stmt;
    __alloc->dest = __var = new variable;
    __store->dst  = __var;

    top->emplace_new(__alloc);

    __var->name = __p->unique_name + ".addr";
    __var->type = ++get_type(__p->type);
    result = variable_map[__p] = __var;
    return __store;
}


void IRbuilder::make_basic(scope *__string,scope *__array) {
    /**
     * __array__::size(this)                = 0
     * string::length(this)                 = 1
     * string::substring(this,int l,int r)  = 2
     * string::parseInt(this)               = 3
     * string::ord(this,int n)              = 4
     * ::print(string str)                  = 5
     * ::println(string str)                = 6
     * ::printInt(int n)                    = 7
     * ::printlnInt(int n)                  = 8
     * ::getString()                        = 9
     * ::getInt()                           = 10
     * ::toString(int n)                    = 11
     * ::__string__add__(string a,string b) = 12
     * ::__string__cmp__(string a,string b) = 13
     * 
    */
    builtin_function.resize(14);

    class_map["bool"] = &__boolean_class__;
    class_map["null"] = &__null_class__;

    wrapper __str = {class_map["string"] = new string_type {} ,1};
    wrapper __i32 = {class_map["int"]    = &__integer_class__ ,0};
    wrapper __voi = {class_map["void"]   = &   __void_class__ ,0};

    builtin_function[0].type = __i32;
    builtin_function[1].type = __i32;
    builtin_function[2].type = __str;
    builtin_function[3].type = __i32;
    builtin_function[4].type = __i32;
    builtin_function[5].type = __voi;
    builtin_function[6].type = __voi;
    builtin_function[7].type = __voi;
    builtin_function[8].type = __voi;
    builtin_function[9].type = __str;
    builtin_function[10].type = __i32;
    builtin_function[11].type = __str;
    builtin_function[12].type = __str;
    builtin_function[13].type = __i32;

    builtin_function[0].name = "__array__::size";
    builtin_function[1].name = "string::length";
    builtin_function[2].name = "string::substring";
    builtin_function[3].name = "string::parseInt";
    builtin_function[4].name = "string::ord";
    builtin_function[5].name = "::print";
    builtin_function[6].name = "::println";
    builtin_function[7].name = "::printInt";
    builtin_function[8].name = "::printlnInt";
    builtin_function[9].name = "::getString";
    builtin_function[10].name = "::getInt";
    builtin_function[11].name = "::toString";
    builtin_function[12].name = "::__string__add__";
    builtin_function[13].name = "::__string__cmp__";

    auto *__builtin = builtin_function.data();
    function_map[__array->find("size")]             = __builtin + 0;
    function_map[__string->find("length")]          = __builtin + 1;
    function_map[__string->find("substring")]       = __builtin + 2;
    function_map[__string->find("parseInt")]        = __builtin + 3;
    function_map[__string->find("ord")]             = __builtin + 4;
    function_map[global_scope->find("print")]       = __builtin + 5;
    function_map[global_scope->find("println")]     = __builtin + 6;
    function_map[global_scope->find("printInt")]    = __builtin + 7;
    function_map[global_scope->find("printlnInt")]  = __builtin + 8;
    function_map[global_scope->find("getString")]   = __builtin + 9;
    function_map[global_scope->find("getInt")]      = __builtin + 10;
    function_map[global_scope->find("toString")]    = __builtin + 11;

    runtime_assert("How can this happen......Fuck!",function_map.find(nullptr) == function_map.end());
}

}
