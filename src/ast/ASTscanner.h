#pragma once

#include "utility.h"
#include "ASTnode.h"
#include "scope.h"
#include <map>


namespace dark::AST {

struct ASTClassScanner {

    /* Add support for basic types. */
    static std::map <std::string,typeinfo *>
        make_basic(std::map <std::string,typeinfo *> &__map) {
        std::map <std::string,typeinfo *> __ans;
        decltype(__ans.end()) __iter;

        /* int */
        if(__iter = __map.find("int") ; __iter == __map.end()) {
            __ans.insert({"int",new typeinfo {nullptr,"int"}});
        } else {
            __ans.insert(std::move(*__iter));
            __map.erase(__iter);
        }

        /* bool */
        if(__iter = __map.find("bool") ; __iter == __map.end()) {
            __ans.insert({"bool",new typeinfo {nullptr,"bool"}});
        } else {
            __ans.insert(std::move(*__iter));
            __map.erase(__iter);
        }

        /* void */
        if(__iter = __map.find("void") ; __iter == __map.end()) {
            __ans.insert({"void",new typeinfo {nullptr,"void"}});
        } else {
            __ans.insert(std::move(*__iter));
            __map.erase(__iter);
        }

        /* string */
        if(__iter = __map.find("string") ; __iter == __map.end()) {
            __ans.insert({"string",new typeinfo {nullptr,"string"}});
        } else {
            __ans.insert(std::move(*__iter));
            __map.erase(__iter);
        }

        /* Create null type and array type. */ 
        __ans.insert({"null",new typeinfo {nullptr,"null"}});
        __ans.insert({"__array__",new typeinfo {nullptr,"__array__"}});

        /* Add some member functions. */
        make_member(__ans);
        return __ans;
    }


    /* Add member functions for arrays and string. */
    static void make_member(std::map <std::string,typeinfo *> &__map) {
        auto &&__int_type = wrapper {__map["int"],0,0};
        { /* Array type ::size() */
            auto *__temp = new scope {nullptr};
            auto *__func = new function;
            __func->name = "size";
            __func->type = __int_type;
            __func->unique_name = "__array__::size";
            __temp->insert("size",__func);
            __map["__array__"]->space = __temp;
        }
        { /* String type  */
            auto *__temp = new scope {nullptr};
            { /* ::length() */
                auto *__func = new function;
                __func->name = "length";
                __func->type = __int_type;
                __func->unique_name = "string::length";
                __temp->insert("length",__func);
            }
            { /* ::substring(int l,int r) */
                auto *__func = new function;
                __func->name = "substring";
                __func->type = {__map["string"],0,0};
                __func->args = {{"l",__int_type},{"r",__int_type}};
                __func->unique_name = "string::substring";
                __temp->insert("substring",__func);
            }
            { /* ::parseInt() */
                auto *__func = new function;
                __func->name = "parseInt";
                __func->type = __int_type;
                __func->unique_name = "string::parseInt";
                __temp->insert("parseInt",__func);
            }
            { /* ::ord(int n) */
                auto *__func = new function;
                __func->name = "ord";
                __func->type = __int_type;
                __func->args = {{"n",__int_type}};
                __func->unique_name = "string::ord";
                __temp->insert("ord",__func);
            }
            __map["string"]->space = __temp;
        }
    }


    /**
     * @brief This function will scan the class definitions and
     * add all classes , basic types and builtin member functions 
     * to the map. In the end, it will clear the __map.
     * 
     * @return A map of all class info.
    */
    static std::map <std::string,typeinfo *> scan
        (const std::vector <definition *>  &__def,
         std::map <std::string,typeinfo *> &__map) {
        auto __ans = make_basic(__map);
        for(auto __p : __def) {
            auto *__tmp = dynamic_cast <class_def *> (__p);
            if (! __tmp) continue;
            auto __iter = __map.find(__tmp->name);
            if(__iter == __map.end()) {
                /// This is an class that is never refered!
                /// TODO: optimize it out.

                typeinfo *__class = new typeinfo {nullptr,__tmp->name};
                if(!__ans.insert({__class->name,__class}).second)
                    throw error("Duplicate class name!");
                std::cout << "Warning: Unused type \"" << __tmp->name  << "\"\n";
            } else {
                // Simply move the data within.
                if(!__ans.insert(std::move(*__iter)).second)
                    throw error("Duplicate class name!");
                __map.erase(__iter);
            }
        }

        /* Check all types used in the program. */
        std::string __error = "";
        for(auto &&[__name,__p] : __map) __error += '\"' + __name + "\" ";
        if(__error != "") throw error("Undefined type: " + __error);

        /* Now the memory in the map has become useless. */
        __map.clear();
        /* Add 4 basic types. */
        return __ans;
    }


    ASTClassScanner() = delete;

};


struct ASTFunctionScanner {

    /* Add basic support for global functions. */
    static scope *make_basic(const std::map <std::string,typeinfo *> &__map) {
        auto &&__int_type    = wrapper {__map.find("int")->second,0,0};
        auto &&__void_type   = wrapper {__map.find("void")->second,0,0};
        auto &&__string_type = wrapper {__map.find("string")->second,0,0};
        auto *__temp = new scope {nullptr};
        { /* ::print(string str) */
            auto *__func = new function;
            __func->name = "print";
            __func->type = __void_type;
            __func->args = {{"str",__string_type}};
            __func->unique_name = "::print";
            __temp->insert("print",__func);
        }
        { /* ::println(string str) */
            auto *__func = new function;
            __func->name = "println";
            __func->type = __void_type;
            __func->args = {{"str",__string_type}};
            __func->unique_name = "::println";
            __temp->insert("println",__func);
        }
        { /* ::printInt(int n) */
            auto *__func = new function;
            __func->name = "printInt";
            __func->type = __void_type;
            __func->args = {{"n",__int_type}};
            __func->unique_name = "::printInt";
            __temp->insert("printInt",__func);
        }
        { /* ::printlnInt(int n) */
            auto *__func = new function;
            __func->name = "printlnInt";
            __func->type = __void_type;
            __func->args = {{"n",__int_type}};
            __func->unique_name = "::printlnInt";
            __temp->insert("printlnInt",__func);
        }
        { /* ::getString() */
            auto *__func = new function;
            __func->name = "getString";
            __func->type = __string_type;
            __func->unique_name = "::getString";
            __temp->insert("getString",__func);
        }
        { /* ::getInt() */
            auto *__func = new function;
            __func->name = "getInt";
            __func->type = __int_type;
            __func->unique_name = "::getInt";
            __temp->insert("getInt",__func);
        }
        { /* ::toString(int n) */
            auto *__func = new function;
            __func->name = "toString";
            __func->type = __string_type;
            __func->args = {{"n",__int_type}};
            __func->unique_name = "::toString";
            __temp->insert("toString",__func);
        }
        return __temp;
    }


    /**
     * @brief Due to some reason, this function only
     * checks whether the param of the function is void.
     * The type check has been done in class scanner.
     * 
     * @return Whether this function pass the test.
    */
    static bool check_void(function *__func,
                           const std::map <std::string,typeinfo *> &__map) {
        auto __void = __map.find("void")->second;

        /* Check whether the return type is void array. */
        wrapper &__return = __func->type;
        if(__return.type == __void && __return.dimension() > 0)
            return false;

        /* Check whether the param is void now. */
        for(auto &__p : __func->args)
            if(__p.type.type == __void)
                return false;

        /* Sucess! */
        return true;
    }

    /**
     * @brief This function will scan the function definitions.
     * It will check the params of the functions and member functions.
     * In addition, it will set up the scope for class types.
     * 
     * @throw dark::error
     * @return The global scope that is created.
    */
    static scope *scan
        (const std::vector <definition *>        &__def,
         const std::map <std::string,typeinfo *> &__map) {
        scope *__ans = make_basic(__map);
        /* Build scope for class. */
        for(auto __p : __def) {
            /* Class case. */
            if (auto *__class = dynamic_cast <class_def *> (__p); __class) {
                /* First create a scope. */
                auto *__type  = __map.find(__class->name)->second;
                __type->space =__class->space = new scope {__ans};
                
                /* Visiting all class members. */
                for(auto __t : __class->member) {
                    /* Member function case. */
                    if(auto *__func = dynamic_cast <function *> (__t); __func) {
                        /* This will check the function's argument type and name. */
                        assert_member(__func,__class->space,__map);

                        /* Check the constructor. */
                        if(__func->name.empty()) {
                            if(__func->type.name() != __class->name)
                                throw error("Invalid class constructor",__func);
                            __func->type = {__map.find("void")->second,0,0};
                        }
                        __func->space = new scope {__class->space};

                        /* Then add this poibter to the member functions. */
                        auto *__ptr   = new variable;
                        __ptr->name   = "this";
                        __ptr->type   = {.type = __type,.info = 0,.flag = false};
                        __ptr->unique_name = __class->name + "::this";
                        __func->space->insert("this",__ptr);
                    }

                    /* Member variable case.  */
                    else {
                        auto *__list = dynamic_cast <variable_def *> (__t);
                        if(!__list) throw error("This should never happen!");
                        for(auto &&[__name,__init] : __list->init) {
                            /* No initialization list. */
                            if(__init != nullptr)
                                throw error("Member variable can't possess initialization statement!");

                            /* Check for identical variable. */
                            auto *__var = new variable;
                            __var->name = __name;
                            __var->type = __list->type;
                            __var->type.flag = true;
                            __var->unique_name = __class->name + "::" + __name;

                            if(!__class->space->insert(__name, __var))
                                throw error("Duplicated variable name: \"" + __name + '\"');
                        }
                    }
                }
            }

            /* Function case. */
            else if (auto *__func = dynamic_cast <function *> (__p); __func) {
                assert_global(__func,__ans,__map);
                __func->space       = new scope {__ans};
                __func->unique_name = "::" + __func->name;
            }
        }

        return __ans;
    }

    
    /**
     * @brief Check member function.
     * It will also insert the function name to current scope.
     * 
     * @param __func Member function node pointer.
     * @param __cur  Current scope.
     * @param __map  Mapping of all class information.
     * @throw dark::error
    */
    static void assert_member(function *__func, scope *__cur,
                              const std::map <std::string,typeinfo *> &__map) {
        if(!check_void(__func,__map))
            throw error("Function arguments can't be void type!");
        if(!__cur->insert(__func->name,__func))
            throw error("Duplicated member function name: \""  + __func->name + '\"');
    }


    /**
     * @brief Check global function.
     * It will also insert the function name to current scope.
     * 
     * @param __func Global function node pointer.
     * @param __cur  Current scope.
     * @param __map  Mapping of all class information.
     * 
     * @throw dark::error
    */
    static void assert_global(function *__func, scope *__cur,
                              const std::map <std::string,typeinfo *> &__map) {
        if(!check_void(__func,__map))
            throw error("Function arguments cannot be void type!");
        if(__map.count(__func->name))
            throw error("Duplicated function & class name: \"" + __func->name + '\"');
        if(!__cur->insert(__func->name,__func))
            throw error("Duplicated global function name: \"" + __func->name + '\"');
    }


    ASTFunctionScanner() = delete;

};


struct ASTTypeScanner {
    /* Check the operator for bool type. */
    static std::string assert_bool(binary_expr *ctx) {
        switch(ctx->op[0]) {
            case '&':
            case '|':
                if(ctx->op[1]) break;
            case '>':
            case '<':
            case '+':
            case '-':
            case '*':
            case '/':
            case '%':
            case '^':
                throw error(
                    std::string("No such operator ") 
                    + ctx->op.str
                    + " for type \"bool\".",ctx
                );
        } return "bool";
    }


    /* Check the operator for int type. */
    static std::string assert_int(binary_expr *ctx) {
        if(ctx->op[0] == ctx->op[1]
            && ctx->op[0] == '&' || ctx->op[0] == '|') {
            throw error(
                std::string("No such operator \"")
                + ctx->op.str
                + "\" for type \"int\".",ctx
            );
        }
        /* Specially, comparison operator will be updated. */
        switch(ctx->op[0]) {
            case '<': case '>': if(ctx->op[1] == ctx->op[0]) break;
            case '!': case '=':
                return "bool";
        } return "int";
    }


    /* Check the operator for bool type. */
    static std::string assert_string(binary_expr *ctx) {
        switch(ctx->op[0]) {
            case '>':
            case '<':
                if(ctx->op[0] != ctx->op[1])
                    break;
            case '&':
            case '|':
            case '-':
            case '*':
            case '/':
            case '%':
            case '^':
                throw error(
                    std::string("No such operator ") 
                    + ctx->op.str
                    + " for type \"string\".",ctx
                );
        }

        /* Only addition returns a string. */
        switch(ctx->op[0]) {
            case '+': return "string";
            default : return "bool";
        }
    }


    /* Check the operator for bool type. */
    static std::string assert_null(binary_expr *ctx) {
        throw error(
            std::string("No such operator ")
            + ctx->op.str
            + " for \"null\".",ctx
        );
    }


    /* Check the operator for bool type. */
    static std::string assert_class(binary_expr *ctx) {
         /* No operator other than != and == and for them. */
        if(ctx->op[0] == '=' || ctx->op[0] == '!') {
            return "bool";
        } else {
            throw error(
                std::string("No such operator ") 
                + ctx->op.str
                + " for type \""
                + ctx->lval->name()
                + "\".",ctx
            );
        }
    }

    ASTTypeScanner() = delete;
};


}
