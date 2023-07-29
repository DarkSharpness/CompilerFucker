#pragma once

#include <iostream>

namespace dark {

struct error {
    error(std::string __s) { std::cerr << __s << '\n'; }
};


struct op_type {
  private:
    char str[8] = {0};
  public:
    char &operator[](size_t __n)       { return str[__n]; }
    char  operator[](size_t __n) const { return str[__n]; }

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

    op_type &operator =(char *__s) noexcept {
        for(int i = 0 ; i < 8 && __s[i] ; ++i)
            str[i] = __s[i];
        return *this;
    }

    op_type &operator =(const char *__s) noexcept {
        for(int i = 0 ; i < 8 && __s[i] ; ++i)
            str[i] = __s[i];
        return *this;
    }

    op_type &operator =(const std::string &__s) noexcept {
        for(int i = 0 ; i < 8 && __s[i] ; ++i)
            str[i] = __s[i];
        return *this;
    }

};


inline std::ostream &operator <<(std::ostream &__os,const op_type &__op) {
    for(int i = 0 ; i < 8 && __op[i] ; ++i)
        std::cout << __op[i];
    return __os;
}


namespace AST {

struct ASTvisitorbase;


struct scope;
struct typeinfo;
struct identifier;
struct wrapper;
struct argument;
struct expression;
struct bracket_expr;
struct subscript_expr;
struct function_expr;
struct unary_expr;
struct member_expr;
struct construct_expr;
struct binary_expr;
struct condition_expr;
struct atom_expr;
struct literal_constant;
struct statement;
struct for_stmt;
struct flow_stmt;
struct while_stmt;
struct block_stmt;
struct branch_stmt;
struct simple_stmt;
struct definition;
struct variable_def;
struct function_def;
struct class_def;

/* AST node holder. */
struct node {
    /* Pointer to the scope that the node belongs. */
    scope *space = nullptr;
    virtual void print() = 0;
    virtual void accept(ASTvisitorbase *) = 0;
    virtual ~node() = default;
};


/* The actual definition and details of a type. */
struct typeinfo {
    std::string name;

    virtual void print() {
        std::cout << "Type info base class!" << std::endl;
    }

    virtual ~typeinfo() = default;
};


/* Wrapper of a specific type with dimension and other infos */
struct wrapper {
    typeinfo *type; /* Pointer to real type. */
    int       info; /* Dimension.            */
    int       flag; /* Whether assignable.   */

    /* Dimension of the object. */
    size_t dimension() const { return info; }

    /* Whether the expression is assignable. */
    bool assignable()  const { return flag; }

    /* This will return the inner type name. */
    const std::string &name() const noexcept {
        return type->name;
    }

    /* Return the full name of the wrapper. */
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


/* An identifier is a function or variable. */
struct identifier {
    virtual ~identifier() = default;
};


/* Argument with name only. */
struct argument {
    std::string name; /* Name of the argument. */
    wrapper     type; /* Type of the variable. */
};


/* Abstract expression wrapping. */
struct expression : node {
    void print() override = 0;
    ~expression() override = default;
};


/* Abstract statement wrapping. */
struct statement : node {
    void print() override = 0;
    ~statement() override = default;
};


/* Abstract definition wrapping. */
struct definition : node {
    void print()  override = 0;
    ~definition() override = default;
};


/* Real variable. */
struct variable : argument , identifier {
    ~variable() override = default;
};
/* Real function. */
using function = function_def;

/* Class definition */
struct class_type : typeinfo {
    scope *space = nullptr; /* Pointer to inner scope. */

    void print() override {
        std::cout << "Class_type" << name << '\n';
    }

    ~class_type() override = default;
};


/* Basic definition. */
struct basic_type : typeinfo {
    void print() override {
        std::cout << "Basic_type: " << name << '\n';
    }

    ~basic_type() override = default;
};



struct ASTvisitorbase {
    virtual void visit(node *ctx) { return ctx->accept(this); }
    virtual void visitBracketExpr(bracket_expr *) = 0;
    virtual void visitSubscriptExpr(subscript_expr *) = 0;
    virtual void visitFunctionExpr(function_expr *) = 0;
    virtual void visitUnaryExpr(unary_expr *) = 0;
    virtual void visitMemberExpr(member_expr *) = 0;
    virtual void visitConstructExpr(construct_expr *) = 0;
    virtual void visitBinaryExpr(binary_expr *) = 0;
    virtual void visitConditionExpr(condition_expr *) = 0;
    virtual void visitAtomExpr(atom_expr *) = 0;
    virtual void visitLiteralConstant(literal_constant *) = 0;
    virtual void visitForStmt(for_stmt *) = 0;
    virtual void visitFlowStmt(flow_stmt *) = 0;
    virtual void visitWhileStmt(while_stmt *) = 0;
    virtual void visitBlockStmt(block_stmt *) = 0;
    virtual void visitBranchStmt(branch_stmt *) = 0;
    virtual void visitSimpleStmt(simple_stmt *) = 0;
    virtual void visitVariable(variable_def *) = 0;
    virtual void visitFunction(function_def *) = 0;
    virtual void visitClass(class_def *) = 0;
};


using argument_list   = std::vector <argument>;
using variable_list   = std::vector <std::pair <std::string,expression *>>;
using expression_list = std::vector <expression *>;

}



}
