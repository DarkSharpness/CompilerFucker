#pragma once

#include <map>
#include <vector>
#include <cstring>
#include <iostream>
#include <stddef.h>


namespace dark::AST {


struct node;
struct argument;
struct object;
struct statement;
struct variable;
struct expression;
struct typeinfo;


using scope   = std::map <std::string,typeinfo *>;


struct op_type {
    char str[8] = {0};
    char operator[](size_t __n) const { return str[__n]; }
    op_type() = default;
    op_type(const op_type &) = default;

    op_type(char *__s) noexcept {
        for(int i = 0 ; i < 8 && __s[i] ; ++i)
            str[i] = __s[i];
    }

    op_type(const char *__s) noexcept {
        for(int i = 0 ; i < 8 && __s[i] ; ++i)
            str[i] = __s[i];
    }

    op_type(const std::string &__s) noexcept : op_type(__s.data()) {}
};


std::ostream operator <<(std::ostream &__os,const op_type &__op) {
    for(int i = 0 ; i < 8 && __op[i] ; ++i)
        std::cout << __op[i];
}


/* Node with  */
struct node {
    // size_t pos;
    virtual void print() = 0;
    virtual ~node() = 0;
};


struct typeinfo : node {
    std::string name;
    void print() override = 0;
    virtual ~typeinfo() = 0;
};


/* Wrapper of a specific type. */
struct wrapper {
    typeinfo *type; /* Pointer to real type. */
    int       info; /* Dimension.            */
    int       flag; /* Whether assignable.   */

    /* Dimension of the object. */
    size_t dimension() const { return info; }

    /* Whether the expression is assignable. */
    bool assignable()  const { return flag; }

    /* Return the info of the wrapper. */
    std::string data() const noexcept {
        std::string __ans;
        size_t __len = type->name.size();
        __ans.resize(__len + info * 2);
        strcpy(__ans.data(),type->name.data());
        for(int i = 0 ; i < info ; ++i) {
            __ans[__len + (i << 1)    ] = '[';
            __ans[__len + (i << 1 | 1)] = ']';
        } return __ans;
    }
};


/* Argument with name only. */
struct argument {
    wrapper     type; /* Type of the variable. */
    std::string name; /* Name of the argument. */
};


/* Variable definition. */
struct variable : node , argument {
    expression *init_list = nullptr;

    void print() override {
        std::cout
            << "Variable signature:\n"
            << type.data() << ' ' << name;
        if(init_list) {
            std::cout << "Initialization list: ";
            init_list->print();
        }
        std::cout << std::endl;
    }

    ~variable() override { delete init_list; }
};


struct expression : node {
    void print() override = 0;
    ~expression() override = 0;
};


struct bracket_expr : expression {
    expression *expr = nullptr;

    void print() override {
        std::cout << '(';
        expr->print();
        std::cout << ')';
    }
    ~bracket_expr() override { delete expr; }
};


struct subscript_expr : expression {
    std::vector <expression *> subs;
    expression *     expr = nullptr;

    void print() override {
        expr->print();
        for(auto __p : subs) {
            std::cout << '[';
            __p->print();
            std::cout << ']';
        }
    }

    ~subscript_expr() override {
        for(auto __p : subs) delete __p;
        delete expr;
    }
};


struct function_expr : expression {
    std::vector <expression *> args;
    expression *     body = nullptr;

    void print() override {
        body->print();
        std::cout << '(';
        bool tag = false;
        for(auto __p : args) {
            if(!tag) { tag = true; }
            else std::cout << ',';
            __p->print();
        } std::cout << ')';
    }

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
            std::cout << op[0] << op[1];
        } else {
            std::cout << op[0];
            if(op[1]) std::cout << op[1];
            expr->print();
        }
    }

    ~unary_expr() override { delete expr; }
};


struct construct_expr : expression {
    wrapper type; /* Wrapper of new type. */
    std::vector <expression *> expr;

    void print() override {
        std::cout << "new " << type.data();

        size_t i = 0;
        size_t __len = expr.size();
        while(i < __len) {
            std::cout << '[';
            expr[i]->print();
            std::cout << ']';
        }

        __len = type.dimension();
        while(i < __len)
            std::cout << '[' << ']';
        std::cout << std::endl;
    }

    ~construct_expr() override { for(auto __p : expr) delete __p; }
};


struct binary_expr : expression {
    expression *lhs = nullptr; /* Left  hand side. */
    expression *rhs = nullptr; /* Right hand side. */
    op_type      op;           /* Operator. */

    void print() override {
        lhs->print();
        std::cout << ' ' << op << ' ';
        rhs->print();
    }

    ~binary_expr() override { delete lhs; delete rhs; }
};


struct condition_expr : expression {
    expression *cond = nullptr; /* Condition.      */
    expression *lval = nullptr; /* Value if true . */
    expression *rval = nullptr; /* Value if false. */

    void print() override {
        cond->print();
        std::cout << " ? ";
        lval->print();
        std::cout << " : ";
        rval->print();
    }

    ~condition_expr() override { delete cond; delete lval; delete rval; }
};


struct literal_constant : expression {
    std::string name;

    void print() override { std::cout << name; }
    ~literal_constant() override = default;
};


struct identifier : expression {
    std::string name;

    void print() override { std::cout << name; }
    ~identifier() override = default;
};


/* Abstract class wrapping. */
struct statement : node {
    void print() override = 0;
    ~statement() override = 0;
};


struct for_stmt : statement {
    expression *init = nullptr;
    expression *cond = nullptr;
    expression *step = nullptr;
    statement  *stmt = nullptr;

    void print() override {
        std::cout << "for ( ";
        init->print();
        std::cout << " ; ";
        cond->print();
        std::cout << " ; ";
        step->print();
        std::cout << " )\n";
        stmt->print();
    }

    ~for_stmt() override { delete init; delete cond; delete step; delete stmt; }
};


struct flow_stmt : statement {
    op_type flow; /* BREAK | RETURN | CONTINUE */
    expression *value = nullptr; /* Return value if return. */

    void print() override {
        std::cout << flow << ' ';
        if(value) value->print();
        std::cout << ';' << std::endl;
    }

    ~flow_stmt() override { delete value; }
};


struct while_stmt : statement {
    expression *cond = nullptr;
    statement  *stmt = nullptr;

    void print() override {
        std::cout << "while ( ";
        cond->print();
        std::cout << " )\n";
        stmt->print();
    }

    ~while_stmt() override { delete cond; delete stmt; }
};


struct block_stmt : statement {
    std::vector <statement *> stmt;

    void print() override {
        std::cout << "{\n";
        for(auto __p : stmt)
            __p->print();
        std::cout << "}\n";
    }

    ~block_stmt() override { for(auto __p : stmt) delete __p; }
};


struct branch_stmt : statement {
    std::vector <expression *> cond;
    std::vector <statement  *> stmt;

    void print() override {
        std::cout << "if ( ";
        cond[0]->print();
        std::cout << " )\n";
        stmt[0]->print();
        for(size_t i = 1 ; i < cond.size() ; ++i) {
            std::cout << "else if ( ";
            cond[i]->print();
            std::cout << " )\n";
            stmt[i]->print();
        }
        if(stmt.size() != cond.size()) {
            std::cout << "else\n";
            stmt.back()->print();
        }
    }

    ~branch_stmt() override {
        for(auto __p : cond) delete __p;
        for(auto __p : stmt) delete __p;
    }
};


struct simple_stmt : statement {
    std::vector <expression *> expr;

    void print() override {
        expr[0]->print();
        for(size_t i = 1 ; i < expr.size() ; ++i) {
            std::cout << ',';
            expr[i]->print();
        } std::cout << ';' << std::endl;
    }

    ~simple_stmt() override { for(auto __p : expr) delete __p; }
};


/* Function definition. */
struct function : typeinfo {
    argument info; /* Function name and return type. */
    block_stmt * body = nullptr; /* Function body in a block. */
    std::vector <argument> arg_list; /* Argument list. */

    void print() override {
        std::cout
            << "Function signature:\n" 
            << info.type.data() << ' ' << info.name
            << '(';
        for(size_t i = 0 ; i < arg_list.size() ; ++i) {
            if(i != 0) std::cout << ',';
            std::cout 
                << arg_list[i].type.data() << ' '
                << arg_list[i].name;
        }
        std::cout << ")\nFunction body:\n";
        body->print();
    }

    ~function() override { delete body; }
};


/* Class definition. */
struct object : typeinfo {
    function    ctor; /* Contructor function. */
    scope    mapping; /* Mapping from name to variables and functions  */

    typeinfo *get_member(const std::string &__f) const {
        auto iter = mapping.find(__f);
        if(iter == mapping.end()) return nullptr;
        return iter->second; 
    }

    void print() override {
        std::cout << "Class: " << name << '\n';

        std::cout << "Member Variables:\n";
        for(auto &&[__name,__p] : mapping)
            if(dynamic_cast <variable *> (__p))
                __p->print();

        std::cout << "Member Functions:\n";
        for(auto &&[__name,__p] : mapping)
            if(dynamic_cast <function *> (__p))
                __p->print();

        std::cout << "Construction "; ctor.print();
    }

    ~object() override {
        for(auto &&[name,__p] : mapping)
            delete __p;
    }
};


/* Class definition. */
struct basic_type : typeinfo {
    function    ctor; /* Contructor function. */

    void print() override {
        std::cout << "Basic_type: " << name << '\n';
        std::cout << "Construction "; ctor.print();
    }

    ~basic_type() override = default;
};



}