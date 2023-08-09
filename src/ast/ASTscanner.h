#pragma once

#include "utility.h"
#include "ASTnode.h"
#include "scope.h"

#include <map>
#include <set>


namespace dark::AST {

struct ASTClassScanner {

    /* Add support for basic types. */
    static void make_basic(std::map <std::string,typeinfo> &__map) {
        __map.insert({"int",      typeinfo {nullptr,"int"}});
        __map.insert({"bool",     typeinfo {nullptr,"bool"}});
        __map.insert({"void",     typeinfo {nullptr,"void"}});
        __map.insert({"string",   typeinfo {nullptr,"string"}});
        __map.insert({"null",     typeinfo {nullptr,"null"}});
        __map.insert({"__array__",typeinfo {nullptr,"__array__"}});

        /* Add some member functions. */
        make_member(__map);
    }


    /* Add member functions for arrays and string. */
    static void make_member(std::map <std::string,typeinfo> &__map) {
        auto &&__int_type = wrapper {&__map["int"],0,0};
        { /* Array type ::size() */
            auto *__temp = new scope {nullptr};
            auto *__func = new function;
            __func->name = "size";
            __func->type = __int_type;
            __func->unique_name = "__array__::size";
            __temp->insert("size",__func);
            __map["__array__"].space = __temp;
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
                __func->type = {&__map["string"],0,0};
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

            __map["string"].space = __temp;
        }
    }


    /**
     * @brief This function will scan the class definitions and
     * add all classes , basic types and builtin member functions 
     * to the map. In the end, it will clear the __map.
     * 
     * @param __def All definitions.
     * @param __map The map of all used types.
     * @return A map of all class info.
    */
    static std::map <std::string,typeinfo> scan
        (const std::vector <definition *> &__def,
         std::map <std::string,typeinfo>  &__map) {
        make_basic(__map);

        /* The name of all defined types. */
        std::set <std::string_view> name_set = {
            "int","void","bool","string","__array__","null"
        };


        for(auto __p : __def) {
            auto __class = dynamic_cast <class_def *> (__p);
            if (!__class) continue;
            auto [______,__result] = name_set.insert(__class->name);
            if(!__result) throw error("Duplicate class name!");
            __class->space = new scope {.prev = nullptr};

            /* This is warning part */
            auto __iter = __map.find(__class->name); 
            if(__iter == __map.end()) {
                /// This is an class that is never refered!
                /// TODO: optimize it out.

                warning("Unused type \"" + __class->name + "\"\n");
                /* Maybe it should not be inserted...... */
                __map.insert({__class->name,{nullptr,__class->name}});
                /* This is used to mark that this class is unused. */
                __class->space->insert("__unused__",nullptr);
            }
        }

        /* Check all types used in the program. */
        std::string __error = "";
        for(auto &&[__name,__p] : __map) 
            if(!name_set.count(__name))
                __error += '\"' + __name + "\" ";
        if(__error != "") throw error("Undefined type: " + __error);

        return std::move(__map);
    }


    ASTClassScanner() = delete;

};


struct ASTFunctionScanner {

    /* Add basic support for global functions. */
    static scope *make_basic(std::map <std::string,typeinfo> &__map) {
        auto &&__int_type    = wrapper {&__map["int"],0,0};
        auto &&__void_type   = wrapper {&__map["void"],0,0};
        auto &&__string_type = wrapper {&__map["string"],0,0};
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
                           std::map <std::string,typeinfo> &__map) {
        auto __void = &__map["void"];

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
        (const std::vector <definition *> &__def,
         std::map <std::string,typeinfo>  &__map) {
        scope *__ans = make_basic(__map);
        /* Build scope for class. */
        for(auto __p : __def) {
            /* Class case. */
            if (auto *__class = dynamic_cast <class_def *> (__p); __class) {
                /* First create a scope. */
                auto *__type  = &__map[__class->name];
                __type->space = __class->space;
                __type->space->prev = __ans;

                /* Visiting all class members. */
                for(auto __t : __class->member) {
                    /* Member function case. */
                    if(auto *__func = dynamic_cast <function *> (__t); __func) {
                        /* This will check the function's argument type and name. */
                        assert_member(__func,__class->space,__map);

                        /* Function name cannot be class name. */
                        if(__func->name == __class->name)
                            throw error("Function name cannot be class name!");

                        /* Check the constructor. */
                        if(__func->name.empty()) {
                            if(__func->type.name() != __class->name)
                                throw error("Invalid class constructor",__func);
                            /* Update its return type. */
                            __func->type = {&__map["void"],0,0};
                        }

                        __func->space = new scope {.prev = __class->space};
                        __func->unique_name = string_join(__class->name,"::",__func->name);

                        /* Then add this pointer to the function. */
                        auto *__ptr   = new variable;
                        __ptr->name   = "this";
                        __ptr->type   = {.type = __type,.info = 0,.flag = false};
                        __ptr->unique_name = "%this";
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
                __func->unique_name = __func->name;
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
                              std::map <std::string,typeinfo> &__map) {
        if(!check_void(__func,__map))
            throw error("Function arguments can't be void type!");
        if(!__cur->insert(__func->name,__func)) {
            if(__func->name.empty())
                throw error("Duplicated constructor for type: \"" + __func->type.name() + '\"');
            else
                throw error("Duplicated member function name: \""  + __func->name + '\"');
        }
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
                              std::map <std::string,typeinfo> &__map) {
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
        && (ctx->op[1] == '&' || ctx->op[1] == '|')) {
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
        if(ctx->op[0] == '!' || ctx->op[0] == '=') {
            return "bool";
        } else throw error(
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
