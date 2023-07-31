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
        std::cout << '(';
        expr->print();
        std::cout << ')';
    }

    void accept(ASTvisitorbase *__p) override { return __p->visitBracketExpr(this); }

    ~bracket_expr() override { delete expr; }
};


struct subscript_expr : expression {
    /* Use first expression as left expression.*/
    expression_list expr;

    void print() override {
        for(size_t i = 0 ; i != expr.size() ; ++i) {
            if(i) std::cout << '[';
            expr[i]->print();
            if(i) std::cout << ']';
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
        std::cout << '(';
        bool tag = false;
        for(auto __p : args) {
            if(!tag) { tag = true; }
            else std::cout << ',';
            __p->print();
        } std::cout << ')';
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
            std::cout << op[0] << op[1];
        } else {
            std::cout << op[0];
            if(op[1]) std::cout << op[1];
            expr->print();
        }
    }

    void accept(ASTvisitorbase *__p) override { return __p->visitUnaryExpr(this); }


    ~unary_expr() override { delete expr; }
};


struct member_expr : expression {
    expression *lval = nullptr; /* Left hand side.     */
    std::string rval;           /* Name of the member. */

    void accept(ASTvisitorbase *__p) override { return __p->visitMemberExpr(this); }

    void print() override {
        lval->print();
        std::cout << '.' << rval;
    }
};


struct construct_expr : expression {
    wrapper type; /* Wrapper of new type. */
    expression_list expr;

    void print() override {
        std::cout << "new " << type.type->name;

        size_t i = 0;
        size_t __len = expr.size();
        for(; i < __len ; ++i) {
            std::cout << '[';
            expr[i]->print();
            std::cout << ']';
        }

        __len = type.dimension();
        for(; i < __len ; ++i)
            std::cout << '[' << ']';
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
        std::cout << ' ' << op << ' ';
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
        std::cout << " ? ";
        lval->print();
        std::cout << " : ";
        rval->print();
    }

    void accept(ASTvisitorbase *__p) override { return __p->visitConditionExpr(this); }

    ~condition_expr() override { delete cond; delete lval; delete rval; }
};


struct atom_expr : expression {
    std::string name;

    void print() override { std::cout << name; }
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

    void print() override { std::cout << name; }
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
        std::cout << "for (";
        if(init) init->print();
        std::cout << ' ';
        // std::cout << " ; "; Statment already contains ';'
        if(cond) cond->print();
        std::cout << "; ";
        if(step) step->print();
        std::cout << ") ";
        if(stmt) stmt->print();
    }

    void accept(ASTvisitorbase *__p) override { return __p->visitForStmt(this); }


    ~for_stmt() override { delete init; delete cond; delete step; delete stmt; }
};


struct flow_stmt : statement {
    op_type flow; /* BREAK | RETURN | CONTINUE */
    expression *expr = nullptr; /* Return value if return. */

    void print() override {
        print_indent();
        std::cout << flow << ' ';
        if(expr) expr->print();
        std::cout << ';';
    }

    void accept(ASTvisitorbase *__p) override { return __p->visitFlowStmt(this); }

    ~flow_stmt() override { delete expr; }
};


struct while_stmt : statement , loop_type {
    expression *cond = nullptr;
    statement  *stmt = nullptr;

    void print() override {
        print_indent();
        std::cout << "while (";
        cond->print();
        std::cout << ") ";
        stmt->print();
    }

    void accept(ASTvisitorbase *__p) override { return __p->visitWhileStmt(this); }

    ~while_stmt() override { delete cond; delete stmt; }
};


struct block_stmt : statement {
    std::vector <statement *> stmt;

    void print() override {
        if(stmt.empty()) { std::cout << "{}\n"; return; }
        std::cout << "{\n";
        ++global_indent;

        for(auto __p : stmt) {
            __p->print();
            std::cout << '\n';
        }
        --global_indent;
        print_indent();
        std::cout << "}";
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
            if(i != 0) std::cout << "else ";
            if(data[i].cond) {
                std::cout << "if (";
                data[i].cond->print();
                std::cout << ") ";
            }
            data[i].stmt->print();
            std::cout << '\n';
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
            if(i != 0) std::cout << " , ";
            expr[i]->print();
        } std::cout << ';';
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
        std::cout << type.data() << ' ';
        bool flag = false;
        for(auto &&[__name,__p] : init) {
            if(!flag) flag = true;
            else      std::cout << ',';
            std::cout << __name;
            if(!__p) continue;
            std::cout << " = ";
            __p->print();
        } std::cout << ';';
    }

    void accept(ASTvisitorbase *__p) override { return __p->visitVariable(this); }

    ~variable_def() override { for(auto __p : init) delete __p.second;}
};


/* Both function definition and function identifier. */
struct function_def : definition , identifier {
    block_stmt * body = nullptr; /* Function body in a block. */
    std::vector <argument> arg_list; /* Argument list. */

    void print() override {
        print_indent();
        std::cout
            // << "Function signature:\n" 
            << type.data() << ' ' << name
            << '(';
        for(size_t i = 0 ; i < arg_list.size() ; ++i) {
            if(i != 0) std::cout << ',';
            std::cout
                << arg_list[i].type.data() << ' '
                << arg_list[i].name;
        }
        std::cout << ") "
                    //   "Function body:\n"
                      ;
        body->print();
    }

    void accept(ASTvisitorbase *__p) override { return __p->visitFunction(this); }


    ~function_def() override { delete body; }
};


/* Class definition. */
struct class_def : definition {
    std::string name;
    /* This member might include ctors. */
    std::vector <definition *> member;

    void print() override {
        print_indent();
        std::cout << "class " << name << " {\n";
        ++global_indent;
        // std::cout << "Member Variables:\n";
        for(auto __p : member)
            if(dynamic_cast <variable_def *> (__p)) {
                __p->print();
                std::cout << '\n';
            }

        // std::cout << "Member Functions:\n";
        for(auto __p : member)
            if(dynamic_cast <function_def *> (__p)) {
                __p->print();
                std::cout << '\n';
            }

        --global_indent;
        std::cout << "};";
    }

    void accept(ASTvisitorbase *__p) override { return __p->visitClass(this); }


    ~class_def() override {  for(auto __p : member) delete __p; }
};



}

