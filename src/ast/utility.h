#pragma once

#include <iostream>
#include <cstring>
#include <vector>

namespace dark {


inline constexpr size_t __string_length(const char *__str) {
    const char *__tmp = __str;
    while(*__tmp) ++__tmp;
    return __tmp - __str;
}
inline size_t __string_length(const std::string &__str) {
    return __str.length();
}
inline size_t __string_length(char) { return 1; }
template <class ...T>
inline size_t __string_length_sum(const T &...__args) {
    return (__string_length(__args) + ...);
}
inline std::string string_join() = delete;
/* Join strings together , safe and fast ! */
template <class ...T>
inline std::string string_join(T &&...__args) {
    std::string __ans;
    __ans.reserve(__string_length_sum(__args...));
    (__ans += ... += std::forward <T> (__args));
    return __ans;
}



struct scope;

/* Pre declaration part. */
namespace AST {

struct ASTvisitorbase;

/* AST node holder. */
struct node {
    /* Pointer to the scope that the node belongs. */
    scope *space = nullptr;
    virtual void print() = 0;
    virtual void accept(ASTvisitorbase *) = 0;
    virtual ~node() = default;
};


};


/* This is only intended to beautify. */
inline size_t global_indent = 0;

/* 4 space as indent */
inline void print_indent() {
    for(size_t i = 0 ; i != global_indent ; ++i)
        std::cout << "    ";
}

struct error {
    error(std::string __s) { std::cerr << "Fatal error: " << __s << '\n'; }

    error(std::string __s,AST::node *ptr) {
        std::cerr << "Error here:\n\"";
        ptr->print();
        std::cerr << "\"\n";
        std::cerr << "Fatal error: " << __s << '\n';
    }

};


struct op_type {
    char str[8] = {0};
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
/* Real function. */
using function = function_def;




/* The actual definition and details of a type. */
struct typeinfo {
    union {
        function *func ;
        scope    *space;
    };

    /* If the typeinfo has no name, it means that the type is a function. */
    std::string name;

    /* Return whether the type is a function type. */
    bool is_function() const noexcept { return name.empty(); }

    /* Return whether the type is a normal class type. */
    bool is_class() const noexcept { return !name.empty(); }

    /* Return whether the type is trivial (non-class). */
    bool is_trivial() const noexcept {
        return name == "int" || name == "void" || name == "bool";
    }

};


/* Wrapper of a specific type with dimension and other infos */
struct wrapper {
    typeinfo *type; /* Pointer to real type. */
    int       info; /* Dimension.            */
    bool      flag; /* Whether assignable.   */

    /* Dimension of the object. */
    size_t dimension()  const { return info; }

    /* Whether the expression is assignable. */
    bool assignable()   const { return flag; }

    /* Whether the expreession is reference type. */
    bool is_reference() const { return dimension() || !type->is_trivial(); }

    /* This will return the inner type name. */
    const std::string &name() const noexcept { return type->name; }

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

    /* Just check the name and dimension. */
    bool check(std::string_view __name,int __dim) const {
        return type->name == __name && info == __dim;
    }

    /* Just compare the name and dimension. */
    friend bool operator == (const wrapper &lhs,const wrapper &rhs) {
        return lhs.type == rhs.type && lhs.info == rhs.info;
    }

    /* Just compare the name and dimension. */
    friend bool operator != (const wrapper &lhs,const wrapper &rhs) {
        return !(lhs == rhs);
    }

};


/* Argument with name only. */
struct argument {
    std::string name; /* Name of the argument. */
    wrapper     type; /* Type of the variable. */
};


/* An identifier is a function or variable. */
struct identifier : argument {
    std::string unique_name; /* Unique name. */
    virtual ~identifier() = default;
};


/* Abstract expression tagging. */
struct expression : node , wrapper {
    void print()  override = 0;
    ~expression() override = default;
};


/* Abstract statement tagging. */
struct statement : node {
    void print() override = 0;
    ~statement() override = default;
};


/* Abstract definition tagging. */
struct definition : node {
    void print()  override = 0;
    ~definition() override = default;
};


/* Abstract loop tagging. */
struct loop_type { virtual ~loop_type() = default; };


/* Real variable. */
struct variable : identifier {
    ~variable() override = default;
};



struct ASTvisitorbase {
    /* This function should never be overwritten! */
    void visit(node *ctx) { return ctx->accept(this); }
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
