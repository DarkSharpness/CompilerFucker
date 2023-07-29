#pragma once

#include "utility.h"
#include "ASTnode.h"
#include "scope.h"
#include <map>


namespace dark::AST {

struct ASTClassScanner {

    /* Add support for basic types. */
    static std::map <std::string,typeinfo *> make_basic() {
        std::map <std::string,typeinfo *> __map;
        basic_type *__tmp;
        /* int */
        __tmp = new basic_type;
        __tmp->name  = "int";
        __map["int"] = __tmp;
        /* bool */
        __tmp = new basic_type;
        __tmp->name   = "bool";
        __map["bool"] = __tmp;
        /* void */
        __tmp = new basic_type;
        __tmp->name   = "void";
        __map["void"] = __tmp;
        /* string */
        __tmp = new basic_type;
        __tmp->name     = "string";
        __map["string"] = __tmp;
        /* Only contains 4 basic types. */
        return __map;
    }


    /**
     * @brief This function will scan the class definitions and
     * add all classes and basic types to the map.
     * 
     * @return A map of all class info.
    */
    static std::map <std::string,typeinfo *> scan
        (const std::vector <definition *> &__def) {
        auto __map = make_basic();
        for(auto __p : __def) {
            auto *__tmp = dynamic_cast <class_def *> (__p);
            if (! __tmp) continue;
            auto *__class = new class_type;
            __class->name = __tmp->name;
            if(!__map.insert({__class->name,__class}).second)
                throw error("Duplicate class name!");
        }
        return __map;
    }
};

struct ASTFunctionScanner {
    /* Add basic support for functions. */
    static scope *make_basic() {
        auto *__tmp = new scope;
        return __tmp;
    }


    /**
     * @brief Check the arguments of the function.
     * It will not check the name of the function.
     * It will update the function's type pointer.
     * 
     * @return Whether this function pass the test.
    */
    static bool check(function *__func,
                      const std::map <std::string,typeinfo *> &__map) {
        /* Check the return type of the function. */
        auto iter = __map.find(__func->type.name());
        if(iter == __map.end()) return false;
        __func->type.type = iter->second;

        /* Check the argument type of the function. */
        for(auto &&__p : __func->arg_list) {
            auto &&__name = __p.type.name();
            iter = __map.find(__name);
            if(__name == "void" || iter == __map.end()) return false;
            __p.type.type = iter->second;
        }

        /* Sucess! */
        return true;
    }


    /**
     * @brief Check member function.
     * It will also insert the function name to current scope.
     * 
     * @param __func Member function node pointer.
     * @param __cur  Current scope.
     * @param __map  Mapping of all class information.
     * @throw dark::error("Invalid global function.")
    */
    static void assert_member(function *__func, scope *__cur,
                             const std::map <std::string,typeinfo *> &__map) {
        if(!check(__func,__map)                 // Check arguments.
        || !__cur->insert(__func->name,__func)  // Check identical function names.
        )   throw error("Invalid global function.");
    }


    /**
     * @brief Check global function.
     * It will also insert the function name to current scope.
     * 
     * @param __func Global function node pointer.
     * @param __cur  Current scope.
     * @param __map  Mapping of all class information.
     * 
     * @throw dark::error("Invalid global function.")
    */
    static void assert_global(function *__func, scope *__cur,
                             const std::map <std::string,typeinfo *> &__map) {
        if(!check(__func,__map)                 // Check arguments.
        ||  __map.count(__func->name)           // Check identical class & function name.
        || !__cur->insert(__func->name,__func)   // Check identical function names.
        )   throw error("Invalid global function.");
    }


    /**
     * @brief This function will scan the function definitions.
     * It will check the params of the functions and member functions.
     * In addition, it will set up the scope for class types.
     * 
     * @return The global scope that is created.
    */
    static scope *scan
        (const std::vector <definition *>        &__def,
         const std::map <std::string,typeinfo *> &__map) {
        scope *__ans = make_basic(); /* The target scope. */ 
        for(auto __p : __def) {
            /* Class case. */
            if (auto *__class = dynamic_cast <class_def *> (__p); __class) {
                __class->space = new scope{__ans};

                for(auto __t : __class->member) {
                    auto *__func = dynamic_cast <function *> (__t);
                    if (! __func) continue;
                    /* This will check the function's argument type and name. */
                    assert_member(__func,__class->space,__map);
                    /* Pass the check, so link the function's scope to the class. */
                    __func->space = new scope{__class->space};
                }

                /* Set the scope for the class type. */
                static_cast <class_type *> (__map.find(__class->name)->second)
                    ->space = __class->space;
            }

            /* Function case. */
            else if (auto *__func  = dynamic_cast <function *> (__p); __func) {
                assert_global(__func,__ans,__map);
                __func->space = new scope{__ans};
            }
        }
        return __ans;
    }

};


struct ASTvisitor : ASTvisitorbase {
    /* Global scope. */
    scope *global = nullptr;
    /* Global mapping. from name to specific type. */
    std::map <std::string,typeinfo *> class_map;

    /* Find the class info within. */
    typeinfo *find_class(const std::string &str) {
        auto iter = class_map.find(str);
        return iter == class_map.end() ? nullptr : iter->second;
    }

    /* Tries to init from ASTbuilder. */
    ASTvisitor(std::vector <definition *>        &__def,
               std::map <std::string,typeinfo *> &__map) {
        std::cout << "\n\n|---------------Start scanning---------------|\n" << std::endl;
        class_map =    ASTClassScanner::scan(__def);
        global    = ASTFunctionScanner::scan(__def,class_map);

    }

    void visit(node *ctx) { return ctx->accept(this); }
    void visitBracketExpr(bracket_expr *) override{}
    void visitSubscriptExpr(subscript_expr *) override{}
    void visitFunctionExpr(function_expr *) override{}
    void visitUnaryExpr(unary_expr *) override{}
    void visitMemberExpr(member_expr *) override{}
    void visitConstructExpr(construct_expr *) override{}
    void visitBinaryExpr(binary_expr *) override{}
    void visitConditionExpr(condition_expr *) override{}
    void visitAtomExpr(atom_expr *) override{}
    void visitLiteralConstant(literal_constant *) override{}
    void visitForStmt(for_stmt *) override{}
    void visitFlowStmt(flow_stmt *) override{}
    void visitWhileStmt(while_stmt *) override{}
    void visitBlockStmt(block_stmt *) override{}
    void visitBranchStmt(branch_stmt *) override{}
    void visitSimpleStmt(simple_stmt *) override{}
    void visitVariable(variable_def *) override{}
    void visitFunction(function_def *) override{}
    void visitClass(class_def *) override{}

};

}
    