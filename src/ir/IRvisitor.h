#pragma once

#include "utility.h"
#include "IRnode.h"

namespace dark::IR {


struct IRbuilder : AST::ASTvisitorbase {
    scope *global_scope = nullptr;

    std::map <std::string      ,IR::class_type>    class_map;
    std::map <AST::identifier *,IR::function *> function_map;
    std::map <AST::identifier *,IR::variable *> variable_map;

    std::vector <initialization> global_variable;
    std::vector <  function   >  global_function;

    function    *top    = nullptr; /* Current function.*/
    definition  *result = nullptr; /* Last definition used. */

    /* This will transform AST type to real type. */
    static IR::typeinfo get_type(AST::typeinfo *type) {
        if(type->name == "int")  return IR::typeinfo::I32;
        if(type->name == "bool") return IR::typeinfo::I1;
        return IR::typeinfo::PTR;
    }


    /* This will transform AST type to real type. */
    static IR::typeinfo get_type(AST::wrapper &type)
    { return get_type(type.type); }


    IRbuilder(scope *__global,
              std::map <std::string,AST::typeinfo> &__map,
              std::vector <AST::definition *>      &__def)
    : global_scope(__global) {
        for(auto &&[__name,__id] : __global->data) {
            /* Global variable case. */
            auto *__var = dynamic_cast <AST::variable *> (__id);
            if ( !__var ) continue;
            createGlobalVariable(__var);
        }
        for(auto __p : __def) {
            if(auto *__class = dynamic_cast <AST::class_def *> (__p)) {
                createGlobalClass(__class);
            } else  if(auto *__func = dynamic_cast <AST::function *> (__p)) {
                createGlobalFunction(__func);
            }
        }

    }


    void visitBracketExpr(AST::bracket_expr *) override;
    void visitSubscriptExpr(AST::subscript_expr *) override;
    void visitFunctionExpr(AST::function_expr *) override;
    void visitUnaryExpr(AST::unary_expr *) override;
    void visitMemberExpr(AST::member_expr *) override;
    void visitConstructExpr(AST::construct_expr *) override;
    void visitBinaryExpr(AST::binary_expr *) override;
    void visitConditionExpr(AST::condition_expr *) override;
    void visitAtomExpr(AST::atom_expr *) override;
    void visitLiteralConstant(AST::literal_constant *) override;
    void visitForStmt(AST::for_stmt *) override;
    void visitFlowStmt(AST::flow_stmt *) override;
    void visitWhileStmt(AST::while_stmt *) override;
    void visitBlockStmt(AST::block_stmt *) override;
    void visitBranchStmt(AST::branch_stmt *) override;
    void visitSimpleStmt(AST::simple_stmt *) override;
    void visitVariable(AST::variable_def *) override;
    void visitFunction(AST::function_def *) override;
    void visitClass(AST::class_def *) override;

    void createGlobalVariable(AST::variable  *);
    void createGlobalFunction(AST::function  *);
    void createGlobalClass   (AST::class_def *);
    
    void make_basic() {
        class_map["i32"];
        class_map["i1"];
        class_map["ptr"];
    }

    /* This will only be used in member access. */
    class_type *get_class(const std::string &__name) {
        auto __iter = class_map.find('@' + __name);
        if (__iter != class_map.end()) return &__iter->second;

        if(__name == "int")  return &class_map["i32"];
        if(__name == "bool") return &class_map["i1"];
        
        /* String / Array : normal reference type. */
        return &class_map["ptr"];
    }



    /* Get a temporary name from its index. */
    static std::string get_temporary_name(size_t __n)
    { return "%" + std::to_string(__n); }

    /* Create a string in global variable. */
    variable *create_string(const std::string &ctx) {
        static size_t __count = 0;
        auto *__var = new variable;
        __var->name = "@str-" + std::to_string(__count);
        __var->type = typeinfo::PTR;
        auto *__lit = new string_constant;
        __lit->context = ctx;
        global_variable.push_back({__var,__lit});
        return __var;
    }

};

}
