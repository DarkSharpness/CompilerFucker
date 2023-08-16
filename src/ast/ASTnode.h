#pragma once

#include "utility.h"

#include <map>
#include <vector>
#include <cstring>
#include <iostream>
#include <stddef.h>


namespace dark::AST {

struct bracket_expr : expression {
    expression *expr = nullptr;

    void print() override {
        std::cerr << '(';
        expr->print();
        std::cerr << ')';
    }

    void accept(ASTvisitorbase *__p) override { return __p->visitBracketExpr(this); }

    ~bracket_expr() override { delete expr; }
};


struct subscript_expr : expression {
    /* Use first expression as left expression.*/
    expression_list expr;

    void print() override {
        for(size_t i = 0 ; i != expr.size() ; ++i) {
            if(i) std::cerr << '[';
            expr[i]->print();
            if(i) std::cerr << ']';
        }
    }

    void accept(ASTvisitorbase *__p) override { return __p->visitSubscriptExpr(this); }

    ~subscript_expr() override { for(auto __p : expr) delete __p; }

};


struct function_expr : expression {
    expression_list args;
    expression *     body = nullptr;

    void print() override {
        body->print();
        std::cerr << '(';
        bool tag = false;
        for(auto __p : args) {
            if(!tag) { tag = true; }
            else std::cerr << ',';
            __p->print();
        } std::cerr << ')';
    }

    void accept(ASTvisitorbase *__p) override { return __p->visitFunctionExpr(this); }

    ~function_expr() override {
        for(auto __p : args) delete __p;
        delete body;
    }
};


struct unary_expr : expression {
    expression *expr = nullptr;
    op_type op; /* x++/x-- has operator +++/--- */

    void print() override {
        if(op[2] != 0) {
            expr->print();
            std::cerr << op[0] << op[1];
        } else {
            std::cerr << op[0];
            if(op[1]) std::cerr << op[1];
            expr->print();
        }
    }

    void accept(ASTvisitorbase *__p) override { return __p->visitUnaryExpr(this); }


    ~unary_expr() override { delete expr; }
};


struct member_expr : expression {
    expression *lval = nullptr; /* Left hand side.     */
    std::string rval;           /* Name of the member. */
    // identifier *real = nullptr; /* Pointer to the real identifier. */

    void accept(ASTvisitorbase *__p) override { return __p->visitMemberExpr(this); }

    void print() override {
        lval->print();
        std::cerr << '.' << rval;
    }
};


struct construct_expr : expression {
    wrapper type; /* Wrapper of new type. */
    expression_list expr;

    void print() override {
        std::cerr << "new " << type.type->name;

        size_t i = 0;
        for(; i < expr.size() ; ++i) {
            std::cerr << '[';
            expr[i]->print();
            std::cerr << ']';
        }

        for(; i < type.dimension() ; ++i)
            std::cerr << '[' << ']';
    }

    void accept(ASTvisitorbase *__p) override { return __p->visitConstructExpr(this); }


    ~construct_expr() override { for(auto __p : expr) delete __p; }
};


struct binary_expr : expression {
    expression *lval = nullptr; /* Left  hand side. */
    expression *rval = nullptr; /* Right hand side. */
    op_type      op;            /* Operator. */

    void print() override {
        lval->print();
        std::cerr << ' ' << op << ' ';
        rval->print();
    }

    void accept(ASTvisitorbase *__p) override { return __p->visitBinaryExpr(this); }

    ~binary_expr() override { delete lval; delete rval; }
};


struct condition_expr : expression {
    expression *cond = nullptr; /* Condition.      */
    expression *lval = nullptr; /* Value if true . */
    expression *rval = nullptr; /* Value if false. */

    void print() override {
        cond->print();
        std::cerr << " ? ";
        lval->print();
        std::cerr << " : ";
        rval->print();
    }

    void accept(ASTvisitorbase *__p) override { return __p->visitConditionExpr(this); }

    ~condition_expr() override { delete cond; delete lval; delete rval; }
};


struct atom_expr : expression {
    std::string name;           /* Name of the atom expression */
    identifier *real = nullptr; /* Pointer to real identifier. */

    void print() override { std::cerr << name; }
    void accept(ASTvisitorbase *__p) override { return __p->visitAtomExpr(this); }
    ~atom_expr() override = default;
};


struct literal_constant : expression {
    enum {
        NUMBER,
        CSTRING,
        NULL_,
        TRUE,
        FALSE
    } type;

    std::string name;

    literal_constant() = default;

    explicit literal_constant(int __n,wrapper __w) 
    noexcept : type(NUMBER), name(std::to_string(__n)) {
        static_cast <wrapper &> (*this) = __w;
    }

    explicit literal_constant(std::string __str,wrapper __w) 
    noexcept : type(CSTRING), name(std::move(__str)) {
        static_cast <wrapper &> (*this) = __w;
    }

    explicit literal_constant(bool __b,wrapper __w)
    noexcept : type(__b ? TRUE : FALSE), name(__b ? "true" : "false") {
        static_cast <wrapper &> (*this) = __w;
    }

    void print() override {
        if(type == CSTRING) std::cerr << '\"' << name << '\"';
        else std::cerr << name;
    }

    /* Test whether the number is given integer. */
    bool is_integer(int n) noexcept { return type == NUMBER && n == std::stoi(name); }

    void accept(ASTvisitorbase *__p) override { return __p->visitLiteralConstant(this); }
    ~literal_constant() override = default;
};


struct for_stmt : statement , loop_type {
    statement  *init = nullptr;
    expression *cond = nullptr;
    expression *step = nullptr;
    statement  *stmt = nullptr;

    void print() override {
        print_indent();
        std::cerr << "for (";
        size_t __temp = global_indent;
        global_indent = 0;
        if(init) init->print(); /* Avoid the useless indent. */
        global_indent = __temp;
        std::cerr << ' ';
        if(cond) cond->print();
        std::cerr << "; ";
        if(step) step->print();
        std::cerr << ") ";
        if(stmt) stmt->print();
    }

    void accept(ASTvisitorbase *__p) override { return __p->visitForStmt(this); }


    ~for_stmt() override { delete init; delete cond; delete step; delete stmt; }
};


struct flow_stmt : statement {
    op_type flow; /* BREAK | RETURN | CONTINUE */
    expression *expr = nullptr; /* Return value if return. */
    function  *func; /* The location current flow will go. */

    void print() override {
        print_indent();
        std::cerr << flow << ' ';
        if(expr) expr->print();
        std::cerr << ';';
    }

    void accept(ASTvisitorbase *__p) override { return __p->visitFlowStmt(this); }

    ~flow_stmt() override { delete expr; }
};


struct while_stmt : statement , loop_type {
    expression *cond = nullptr;
    statement  *stmt = nullptr;

    void print() override {
        print_indent();
        std::cerr << "while (";
        cond->print();
        std::cerr << ") ";
        stmt->print();
    }

    void accept(ASTvisitorbase *__p) override { return __p->visitWhileStmt(this); }

    ~while_stmt() override { delete cond; delete stmt; }
};


struct block_stmt : statement {
    std::vector <statement *> stmt;

    void print() override {
        if(stmt.empty()) { std::cerr << "{}\n"; return; }
        std::cerr << "{\n";
        ++global_indent;

        for(auto __p : stmt) {
            __p->print();
            std::cerr << '\n';
        }
        --global_indent;
        print_indent();
        std::cerr << "}";
    }

    void accept(ASTvisitorbase *__p) override { return __p->visitBlockStmt(this); }

    ~block_stmt() override { for(auto __p : stmt) delete __p; }
};


struct branch_stmt : statement {
    using pair_t = struct {
        expression *cond = nullptr;
        statement  *stmt = nullptr;
    };

    std::vector <pair_t> data; /* Branch info. */

    void print() override {
        for(size_t i = 0 ; i < data.size(); ++i) {
            print_indent();
            if(i != 0) std::cerr << "else ";
            if(data[i].cond) {
                std::cerr << "if (";
                data[i].cond->print();
                std::cerr << ") ";
            }
            data[i].stmt->print();
            std::cerr << '\n';
        }
    }

    void accept(ASTvisitorbase *__p) override { return __p->visitBranchStmt(this); }

    ~branch_stmt() override { for(auto [__c,__s] : data) delete __c, delete __s; }
};


struct simple_stmt : statement {
    expression_list expr;

    void print() override {
        print_indent();
        for(size_t i = 0 ; i < expr.size() ; ++i) {
            if(i != 0) std::cerr << " , ";
            expr[i]->print();
        } std::cerr << ';';
    }

    void accept(ASTvisitorbase *__p) override { return __p->visitSimpleStmt(this); }

    ~simple_stmt() override { for(auto __p : expr) delete __p; }
};


/* Multiple variable definition. */
struct variable_def : definition , statement {
    using pair_t = std::pair <std::string,expression *>;

    wrapper       type; /* Type info within. */
    variable_list init; /* Initialize list.  */

    void print() override {
        print_indent();
        std::cerr << type.data() << ' ';
        bool flag = false;
        for(auto &&[__name,__p] : init) {
            if(!flag) flag = true;
            else      std::cerr << ',';
            std::cerr << __name;
            if(!__p) continue;
            std::cerr << " = ";
            __p->print();
        } std::cerr << ';';
    }

    void accept(ASTvisitorbase *__p) override { return __p->visitVariable(this); }

    ~variable_def() override { for(auto __p : init) delete __p.second;}
};


/* Both function definition and function identifier. */
struct function_def : definition , identifier {
    block_stmt * body = nullptr; /* Function body in a block. */
    std::vector <argument> args; /* Argument list. */

    /* This unique mapping is intended to store variables. */
    std::vector <variable *> unique_mapping;

    void print() override {
        print_indent();
        std::cerr
            << type.data() << ' ' << name
            << '(';
        for(size_t i = 0 ; i < args.size() ; ++i) {
            if(i != 0) std::cerr << ',';
            std::cerr
                << args[i].type.data() << ' '
                << args[i].name;
        }
        std::cerr << ") ";
        if(!is_builtin()) body->print();
        else std::cerr << " /* Built-in function. */ ;";
    }

    void accept(ASTvisitorbase *__p) override { return __p->visitFunction(this); }

    bool is_builtin() const noexcept { return body == nullptr; }

    // bool is_constructor() const noexcept { return name.empty(); }

    ~function_def() override { delete body; }
};


/* Class definition. */
struct class_def : definition {
    std::string name;
    /* This member might include ctors. */
    std::vector <definition *> member;

    void print() override {
        print_indent();
        std::cerr << "class " << name << " {\n";
        ++global_indent;
        // std::cerr << "Member Variables:\n";
        for(auto __p : member)
            if(dynamic_cast <variable_def *> (__p)) {
                __p->print();
                std::cerr << '\n';
            }

        // std::cerr << "Member Functions:\n";
        for(auto __p : member)
            if(dynamic_cast <function_def *> (__p)) {
                __p->print();
                std::cerr << '\n';
            }

        --global_indent;
        std::cerr << "};";
    }

    void accept(ASTvisitorbase *__p) override { return __p->visitClass(this); }


    ~class_def() override {  for(auto __p : member) delete __p; }
};



}

