#include "IRbuilder.h"
#include "IRconst.h"

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
        __get->dst  = top->create_temporary(__type,"arrayidx.");
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
    if(!is_global_function(__call->func->name))
        __call->args.push_back(result);

 
    for(auto *__p : ctx->args) {
        visit(__p);
        __call->args.push_back(result);
    }

    if(__func->type.name() != "void") {
        __call->dest = top->create_temporary(get_type(*ctx),"call.");
        result = __call->dest;
    }

    top->emplace_new(__call);
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
            string_join(binary_stmt::str[__bin->op],'.')
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
        auto *__tmp = dynamic_cast <literal *> (result);
        if (( __tmp = const_folder(ctx->op)(__tmp))) return void(result = __tmp); 
        if(ctx->op[0] == '+') return;
        if(ctx->op[0] == '!') {
            auto *__cmp = new compare_stmt;
            __cmp->op   = compare_stmt::EQ;
            __cmp->lvar = result;
            __cmp->rvar = __false__;
            __cmp->dest = top->create_temporary(
                wrapper {&__boolean_class__,0},"not."
            );
            top->emplace_new(__cmp);
            return static_cast <void> (result = __cmp->dest);
        }


        auto *__bin = new binary_stmt;
        __bin->op   = ctx->op[0] == '-' ? binary_stmt::SUB : binary_stmt::XOR;
        __bin->dest = top->create_temporary(
            wrapper { (dark::IR::typeinfo *)&__integer_class__,0 },
            string_join(binary_stmt::str[__bin->op],'.')
        );
        __bin->lvar = ctx->op[0] == '-' ? __zero__ : __neg1__;
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


void IRbuilder::visitConstructExpr(AST::construct_expr *ctx) {
    auto  __type = get_type(*ctx);
    auto *__data = ctx->expr.data();
    std::vector <definition *> __args;
    __args.reserve(ctx->expr.size());
    for(auto __iter  = ctx->expr.rbegin() ; 
             __iter != ctx->expr.rend() ; ++__iter) {
        visit(*__iter);
        __args.push_back(result);
    }
    if(__args.size()) return visitNewExpr(__type,__args);
    else {
        {
            auto *__call = new call_stmt;
            __call->func = get_new_object();
            __call->args = { create_integer ((int) (--__type).size()) };
            __call->dest = top->create_temporary(__type,"new.");
            top->emplace_new(__call);
            result = __call->dest;
        }

        auto *__class = safe_cast <const class_type *> (__type.type);
        if(!__class->constructor) return;

        {
            auto *__call = new call_stmt;
            __call->func = __class->constructor;
            __call->args = { result };
            top->emplace_new(__call);
        }
    }
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

    /* Special case. */
    if(ctx->lval->type->name == "string") return visitStringBinary(ctx);

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
            default : throw error("Unknown operator!");
        }
        visit(ctx->lval);
        __bin->lvar = result;
        visit(ctx->rval);
        __bin->rvar = result;

        auto *__lhs = dynamic_cast <integer_constant *> (__bin->lvar);
        auto *__rhs = dynamic_cast <integer_constant *> (__bin->rvar);
        if ((result = const_folder(__bin->op)(__lhs,__rhs))) {
            if (result == &__INTEGER_DIVIDE_BY_ZERO__) {
                /* Possible warning! */
                warning("Undefined behavior: division by zero!");
                top->emplace_new(create_unreachable());
            } delete __bin; return;
        }

        __bin->dest = top->create_temporary(
            {&__integer_class__,0},
            string_join(binary_stmt::str[__bin->op],'.')
        );
        __bin->dest->type = get_type(*ctx);

        top->emplace_new(__bin);
        result = __bin->dest;
        return;
    }

    /* Noraml logical comparasion case. */
    if(ctx->op[0] != '&' && ctx->op[0] != '|') {
        auto *__cmp = new compare_stmt;
        __cmp->op = 
                ctx->op == "==" ? compare_stmt::EQ :
                ctx->op == "!=" ? compare_stmt::NE :
                ctx->op == ">"  ? compare_stmt::GT :
                ctx->op == ">=" ? compare_stmt::GE :
                ctx->op == "<"  ? compare_stmt::LT :
                ctx->op == "<=" ? compare_stmt::LE :
                        throw error("Unknown operator!");
        visit(ctx->lval);
        __cmp->lvar = result;
        visit(ctx->rval);
        __cmp->rvar = result;

        auto *__lhs = dynamic_cast <boolean_constant *> (__cmp->lvar);
        auto *__rhs = dynamic_cast <boolean_constant *> (__cmp->rvar);
        if ((result = const_folder(__cmp->op)(__lhs,__rhs))) { delete __cmp; return; }
    
        __cmp->dest = top->create_temporary({&__boolean_class__,0},"cmp.");
        top->emplace_new(__cmp);
        result = __cmp->dest;
        return;
    }

    /* Short-circuit logic case. */
    bool __is_or = bool(ctx->op[0] == '|');

    visit(ctx->lval);
    auto *__lhs = dynamic_cast <boolean_constant *> (result);
    if(__lhs) return (__is_or == __lhs->value) ?
        void() : visit(ctx->rval);

    std::string __name = top->create_label("logic");
    auto *__br  = new branch_stmt;
    auto *__end = new block_stmt;
    auto *__new = new block_stmt;
    __br->cond  = result;
    __br->br[ __is_or] = __new;
    __br->br[!__is_or] = __end;
    __br->br[ __is_or]->label = __name + '-' + (__is_or ? "false" : "true");
    __br->br[!__is_or]->label = __name + "-end";

    /* Name of previous branch */
    auto *__phi = new phi_stmt;
    __phi->dest = top->create_temporary({&__boolean_class__,0},"phi.");
    __phi->cond.push_back({
        __is_or ? __true__ : __false__ ,
        top->stmt.back()
    });

    top->emplace_new(__br);
    top->emplace_new(__new);

    visit(ctx->rval);

    __phi->cond.push_back({result,top->stmt.back()});

    create_jump(__end);
    top->emplace_new(__end);
    top->emplace_new(__phi);

    result = __phi->dest;
}


void IRbuilder::visitConditionExpr(AST::condition_expr *ctx) {
    visit(ctx->cond);
    std::string __name = top->create_label("cond");

    auto *__br  = new branch_stmt;
    __br->cond  = result;
    __br->br[0] = new block_stmt;
    __br->br[1] = new block_stmt;
    __br->br[0]->label = __name + "-true";
    __br->br[1]->label = __name + "-false";

    top->emplace_new(__br);
    top->emplace_new(__br->br[0]);

    auto *__phi = new phi_stmt;
    auto *__end = new block_stmt;
    __phi->dest = top->create_temporary(get_type(*ctx) + ctx->flag,"ternary.");

    { /* Branch true. */
        visit(ctx->lval);
       __phi->cond.push_back({result,top->stmt.back()});
        create_jump(__end);
    }

    top->emplace_new(__br->br[1]);

    { /* Branch false. */
        visit(ctx->rval);
        __phi->cond.push_back({result,top->stmt.back()});
        create_jump(__end);
    }

    /* If assignable, return the pointer to the data. */
    __end->label = __name + "-end";
    top->emplace_new(__end);
    
    if(ctx->type->name == "void") {
        delete __phi;
    } else {
        top->emplace_new(__phi);
        result = __phi->dest;
    }
}


void IRbuilder::visitAtomExpr(AST::atom_expr *ctx) {
    /* If function, nothing should be done. */
    if(ctx->type->is_function()) {
        auto __func = function_map[ctx->type->func];
        /* Member function case. */
        if(!is_global_function(__func->name))
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
        auto  __cname = __name.substr(0,__len); /* Class name. */
        auto *__class = get_class(__cname);
        auto *__get = new get_stmt;
        __get->src  = get_this_pointer();
        __get->dst  = top->create_temporary(
            ++get_type(*ctx),
            string_join(__cname,'.',ctx->name,'-')
        );
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
            result = create_integer(std::stoi(ctx->name));
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

    std::string __name = top->create_label("for");
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
        create_jump(__cond);

        { /* Build up condition body. */
            top->emplace_new(__cond);
            visit(ctx->cond);
            auto * __br = new branch_stmt;
            __br->cond  = result;
            __br->br[0] = __body;
            __br->br[1] = __end;
            /* Conditional jump to body. */
            top->emplace_new(__br);
        }
    } else { /* Unconditional jump to body. */
        __cond = __body;
        create_jump(__body);
    }

    if(ctx->step) {
        { /* Build the step block first. */
            __step = new block_stmt;
            __step->label = __name + "-step";
            top->emplace_new(__step);
            visit(ctx->step);
        }
        /* Unconditional jump to condition. */
        create_jump(__cond);
    } else { /* No need to jump to step. */
        __step = __cond;
    }

    { /* Build up body. */
        top->emplace_new(__body);
        loop_flow.push_back({.bk = __end,.ct = __step});
        visit(ctx->stmt);
        loop_flow.pop_back();
    }

    /* Unconditionally jump to step. */
    create_jump(__step);
    top->emplace_new(__end);
}


void IRbuilder::visitFlowStmt(AST::flow_stmt *ctx) {
    if(ctx->flow[0] == 'r') {
        auto *__ret = new return_stmt;
        if(ctx->expr) {
            visit(ctx->expr);
            if(ctx->expr->type->name != "void")
                __ret->rval = result;
        }
        __ret->func = function_map[ctx->func];
        top->emplace_new(__ret);
    } else {
        /* Jump to target directly. */
        create_jump(loop_flow.back() [ctx->flow[0] == 'c']);
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

    std::string __name = top->create_label("while");

    /* Jump into condition block. */
    auto *__cond = new block_stmt;
    __cond->label = __name + "-cond";
    create_jump(__cond);
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
    create_jump(__cond);
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
    std::string __name = top->create_label("if");
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
        create_jump(__end);
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
    } else {
        statement *__stmt = nullptr;
        if(top->stmt.size() && top->stmt.back()->stmt.size())
            __stmt = top->stmt.back()->stmt.back();
        if(!dynamic_cast <return_stmt *> (__stmt)
        && !dynamic_cast <branch_stmt *> (__stmt)
        && !dynamic_cast   <jump_stmt *> (__stmt)) {
            if(ctx->unique_name != "main") {
                top->emplace_new(create_unreachable());
            } else {
                auto *__ret = new return_stmt;
                __ret->func = top;
                __ret->rval = __zero__;
                top->emplace_new(__ret);
                // warning (
                //     "Missing return statement in main function\n"
                //     "\"return 0;\" is added by default!",ctx
                // );
            }
        }
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
            /* Constructor function (not empty). */
            if(__func->name == "" && !__func->body->stmt.empty()) {
                __class->constructor = &global_functions.back();
                __class->constructor->name += ctx->name;
            }
        } else {
            auto *__var = safe_cast <AST::variable_def *> (__p);
            auto __info = get_type(__var->type);

            /* Make the member variables. */
            for(auto &&[__name,__init] : __var->init) {
                __class->layout.push_back(__info);
                __class->member.push_back(__name);
                auto *__p   = ctx->space->find(__name);
                auto *__var = new local_variable;
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
    auto *__var = new global_variable;
    __var->name = ctx->unique_name;
    __var->type = ++get_type(ctx->type);
    variable_map[ctx] = __var;

    literal *__lit; /* Literal constant. */
    auto __type = (--__var->type).name();
    if(__type == "i1") {
        __lit = create_boolean (lit && lit->name[0] == 't');
    } else if(__type == "i32") {
        __lit = lit ? create_integer (std::stoi(lit->name)) : __zero__;
    } else if(!lit || lit->type == lit->NULL_) {
        __lit = __null__;
    } else {
        __lit = create_pointer(create_string(lit->name));
    }

    global_variables.push_back({__var,__lit});
}


/* This will be called globally to create a global or member function. */
void IRbuilder::visitGlobalFunction(AST::function_def *ctx) {
    top = &global_functions.emplace_back();
    function_map[ctx] = top;
    top->name = function_name_map(ctx->unique_name);
    top->type = get_type(ctx->type);
    top->emplace_new(new block_stmt);
    top->stmt.front()->label = "entry";

    std::vector <store_stmt *> __store;
    __store.reserve(ctx->args.size() + 1);

    /* Add this pointer to the param of member function. */
    if(!is_global_function(top->name)) {
        auto __this = ctx->space->find("this");
        if (!__this) throw error("Invalid member function!",ctx);
        __store.push_back(visitFunctionParam(__this));
        /* Maybe this pointer should be pre-loaded. */
    }

    /* Add all of the function params. */
    for(auto &&[__name,__type] : ctx->args)
        __store.push_back(visitFunctionParam(ctx->space->find(__name)));

    /* All the local variables used. */
    for(auto *__p : ctx->unique_mapping) {
        auto *__var = new local_variable;
        __var->name = __p->unique_name;
        __var->type = ++get_type(__p->type);
        variable_map[__p] = __var;

        auto *__alloca = new allocate_stmt;
        __alloca->dest = __var;
        top->emplace_new(__alloca);
    }

    /* Set the main function. */
    if(top->name == "main") return void(main_function = top);

    /* Store the function arguments after allocation. */
    for(auto __s : __store) top->emplace_new(__s);
}


store_stmt *IRbuilder::visitFunctionParam(AST::identifier *__p) {
    variable *__var = new function_argument;
    __var->name = __p->unique_name;
    __var->type = get_type(__p->type);
    top->args.push_back(static_cast <function_argument *> (__var));

    auto *__store = new store_stmt;
    __store->src  = __var; /* Variable in the param */

    auto *__alloc = new allocate_stmt;
    __alloc->dest = __var = new local_variable;
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
     * ::__string__cmp__(string a,string b) = 13 ~ 18
     * ::new(int a,int b)                   = 19 ~ 21
    */

    wrapper __str = {class_map["string"] = new string_type {} ,1};
    wrapper __i32 = {class_map["int"]    = &__integer_class__ ,0};
    wrapper __voi = {class_map["void"]   = &   __void_class__ ,0};
    wrapper __boo = {class_map["bool"]   = &__boolean_class__ ,0};
    wrapper __nul = {class_map["null"]   = &   __null_class__ ,0};

    builtin_function.resize(22);
    for(size_t i = 0 ; i < 22 ; ++i)
        builtin_function[i].is_builtin = true;

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
    builtin_function[13].type = __boo;
    builtin_function[14].type = __boo;
    builtin_function[15].type = __boo;
    builtin_function[16].type = __boo;
    builtin_function[17].type = __boo;
    builtin_function[18].type = __boo;
    builtin_function[19].type = __nul;
    builtin_function[20].type = __nul;
    builtin_function[21].type = __nul;

    builtin_function[0].name = "__Array_size__";
    builtin_function[1].name = "strlen";
    builtin_function[2].name = "__String_substring__";
    builtin_function[3].name = "__String_parseInt__";
    builtin_function[4].name = "__String_ord__";
    builtin_function[5].name = "__print__";
    builtin_function[6].name = "puts";
    builtin_function[7].name = "__printInt__";
    builtin_function[8].name = "__printlnInt__";
    builtin_function[9].name = "__getString__";
    builtin_function[10].name = "__getInt__";
    builtin_function[11].name = "__toString__";
    builtin_function[12].name = "__string_add__";
    builtin_function[13].name = "__string_eq__";
    builtin_function[14].name = "__string_ne__";
    builtin_function[15].name = "__string_gt__";
    builtin_function[16].name = "__string_ge__";
    builtin_function[17].name = "__string_lt__";
    builtin_function[18].name = "__string_le__";
    builtin_function[19].name = "__new_array1__";
    builtin_function[20].name = "__new_array4__";
    builtin_function[21].name = "malloc";

    builtin_function[5].inout_state = function::OUT;
    builtin_function[6].inout_state = function::OUT;
    builtin_function[7].inout_state = function::OUT;
    builtin_function[8].inout_state = function::OUT;
    builtin_function[9].inout_state = function::IN;
    builtin_function[10].inout_state = function::IN;

    auto *__ptr__  = new function_argument;
    __ptr__->type  = __nul;
    auto *__int__  = new function_argument;
    __int__->type  = __i32;
    auto *__bool__ = new function_argument;
    __bool__->type = __boo;

    builtin_function[0].args = {__ptr__};
    builtin_function[1].args = {__ptr__};
    builtin_function[2].args = {__ptr__,__int__,__int__};
    builtin_function[3].args = {__ptr__};
    builtin_function[4].args = {__ptr__,__int__};
    builtin_function[5].args = {__ptr__};
    builtin_function[6].args = {__ptr__};
    builtin_function[7].args = {__int__};
    builtin_function[8].args = {__int__};
    builtin_function[9].args = {};
    builtin_function[10].args = {};
    builtin_function[11].args = {__int__};
    builtin_function[12].args = {__ptr__,__ptr__};
    builtin_function[13].args = {__ptr__,__ptr__};
    builtin_function[14].args = {__ptr__,__ptr__};
    builtin_function[15].args = {__ptr__,__ptr__};
    builtin_function[16].args = {__ptr__,__ptr__};
    builtin_function[17].args = {__ptr__,__ptr__};
    builtin_function[18].args = {__ptr__,__ptr__};
    builtin_function[19].args = {__int__};
    builtin_function[20].args = {__int__};
    builtin_function[21].args = {__int__};

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


void IRbuilder::visitStringBinary(AST::binary_expr *ctx) {
    auto *__call = new call_stmt;
    if(ctx->op == "+") {
        __call->func = get_string_add();
    } else {
        decltype (compare_stmt::op) __op =
            ctx->op == "==" ? compare_stmt::EQ :
            ctx->op == "!=" ? compare_stmt::NE :
            ctx->op == ">"  ? compare_stmt::GT :
            ctx->op == ">=" ? compare_stmt::GE :
            ctx->op == "<"  ? compare_stmt::LT :
            ctx->op == "<=" ? compare_stmt::LE :
                throw error("Unknown operator!");
        __call->func = get_string_cmp(__op);
    }
    visit(ctx->lval);
    __call->args.push_back(result);
    visit(ctx->rval);
    __call->args.push_back(result);
    top->emplace_new(__call);
    __call->dest = top->create_temporary(
        ctx->op == "+" ? wrapper {class_map["string"],1}
                       : wrapper {class_map["bool"],0},
        "call."
    );
    result = __call->dest;
}


void IRbuilder::visitNewExpr(wrapper __type,std::vector <definition *> __vec) {
    const std::string &__name = top->create_label("new");
    auto *__len = __vec.back(); __vec.pop_back();

    { /* Allocate the pointer. */
        auto *__call  = new call_stmt;
        __call->args.push_back(__len);
        __call->func  = get_new_array((--__type).size());
        __call->dest  = top->create_temporary_no_suffix(__type,__name + ".ptr");
        top->emplace_new(__call);
        result = __call->dest;
    }

    if(__vec.empty()) return;

    /* Pointer to the first data. */
    definition *__beg  = result;

    { /* Work out the tail pointer first. */
        auto *__get = new get_stmt;
        __get->dst  = top->create_temporary_no_suffix(__type,__name + ".beg");
        __get->src  = __beg;
        __get->idx  = __len;
        __get->mem  = get_stmt::NPOS;
        top->emplace_new(__get);
        result      = __get->dst;
    }

    /* The pointer that will only be used when entering the loop. */
    auto *__pre  = result;
    /* Current pointer that is created by getelementptr. */
    auto *__cur  = top->create_temporary_no_suffix(__type,__name + ".cur");
    /* Temporaray pointer as result of the phi statement. */
    auto *__tmp  = top->create_temporary_no_suffix(__type,__name + ".tmp");

    auto *__last = top->stmt.back();/* Block before body. */
    auto *__body = new block_stmt;  /* Body block.        */
    auto *__cond = new block_stmt;  /* Condition block.   */

    __body->label = __name + "-body";
    create_jump(__cond);
    top->emplace_new(__body);

    /* Work out the next dimension. */
    visitNewExpr(--__type,std::move(__vec));

    { /* Sub the pointer. */
        auto *__get = new get_stmt;
        __get->dst  = __cur;
        __get->src  = __tmp;
        __get->idx  = __neg1__;
        __get->mem  = get_stmt::NPOS;
        top->emplace_new(__get);
    }

    { /* Store the result. */
        auto *__store = new store_stmt;
        __store ->src = result;
        __store ->dst = __cur;
        top->emplace_new(__store);
    }

    __cond->label = __name + "-cond";
    create_jump(__cond);
    top->emplace_new(__cond);

    { /* Use phi to assign. */
        auto *__phi = new phi_stmt;
        __phi->dest = __tmp;
        __phi->cond.push_back({__pre,__last});
        __phi->cond.push_back({__cur,*(++top->stmt.rbegin())});
        top->emplace_new(__phi);
    }

    { /* Compare current pointer and head pointer. */
        auto *__cmp = new compare_stmt;
        __cmp->op   = compare_stmt::NE;
        __cmp->dest = top->create_temporary_no_suffix(
            {&__boolean_class__,0}, /* Result type: bool */
            __name + ".cmp"         /* Compare result. */
        );
        __cmp->lvar = __tmp;
        __cmp->rvar = __beg;
        top->emplace_new(__cmp);
        result = __cmp->dest;
    }

    { /* Branch to initialize. */
        auto *__br  = new branch_stmt;
        __br->cond  = result;
        __br->br[0] = __body;
        __br->br[1] = new block_stmt;    
        __br->br[0]->label = __name + "-body";
        __br->br[1]->label = __name + "-end";
        top->emplace_new(__br);
        top->emplace_new(__br->br[1]);
    }

    result = __beg;
}



}
