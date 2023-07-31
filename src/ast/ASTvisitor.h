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

        __ans.insert({"null",new typeinfo {nullptr,"null"}});
        return __ans;
    }


    /**
     * @brief This function will scan the class definitions and
     * add all classes and basic types to the map.
     * In the end, it will clear the __map.
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

                std::cout << "Warning: Unused type \"" << __tmp->name  << "\"\n";
                typeinfo *__class = new typeinfo {nullptr,__tmp->name};
                if(!__ans.insert({__class->name,__class}).second)
                    throw error("Duplicate class name!");
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


    /**
     * @brief Due to some reason, this function only
     * checks whether the param of the function is void.
     * The type check has been done in class scanner.
     * 
     * @return Whether this function pass the test.
    */
    static bool check_void(function *__func,
                      const std::map <std::string,typeinfo *> &__map) {
        // /* Check the return type of the function. */
        // auto iter = __map.find(__func->type.name());
        // if(iter == __map.end()) return false;
        // __func->type.type = iter->second;

        // /* Check the argument type of the function. */
        // for(auto &&__p : __func->arg_list) {
        //     auto &&__name = __p.type.name();
        //     iter = __map.find(__name);
        //     if(__name == "void" || iter == __map.end()) return false;
        //     __p.type.type = iter->second;
        // }

        auto __void = __map.find("void")->second;

        /* Check whether the return type is void array. */
        wrapper &__return = __func->type;
        if(__return.type == __void && __return.dimension() > 0)
            return false;

        /* Check whether the param is void now. */
        for(auto &__p : __func->arg_list)
            if(__p.type.type == __void)
                return false;

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
            throw error("Function arguments can't be void type!");
        if(__map.count(__func->name))
            throw error("Duplicated function & class name: \"" + __func->name + '\"');
        if(!__cur->insert(__func->name,__func))
            throw error("Duplicated global function name: \"" + __func->name + '\"');
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
        scope *__ans = make_basic();
        for(auto __p : __def) {
            /* Class case. */
            if (auto *__class = dynamic_cast <class_def *> (__p); __class) {
                auto *__type  = __map.find(__class->name)->second;
                __type->space =__class->space = new scope {.prev = __ans};
                auto *__ptr   = new variable;
                __ptr->name   = "this";
                __ptr->type   = {.type = __type,.info = 0,.flag = 1};
                __class->space->insert("this",__ptr);
                for(auto __t : __class->member) {
                    if(auto *__func = dynamic_cast <function *> (__t); __func) {
                        /* This will check the function's argument type and name. */
                        assert_member(__func,__class->space,__map);
                        __func->space = new scope {.prev = __class->space};
                    } else {
                        auto *__list = dynamic_cast <variable_def *> (__t);
                        if(!__list) throw error("This should never happen!");
                        for(auto &&[__name,___] : __list->init) {
                            // No init list!
                            if(___ != nullptr)
                                throw error("Member variable can't possess initialization statement!");
                            // No indentical name.
                            auto *__ptr = new variable;
                            __ptr->name = __name;
                            __ptr->type = __list->type;
                            if(!__class->space->insert(__name, __ptr))
                                throw error("Duplicated variable name: \"" + __name + '\"');
                        }
                    }
                }

                /* Set the scope for real class type. */
                __map.find(__class->name)->second->space = __class->space;
            }

            /* Function case. */
            else if (auto *__func = dynamic_cast <function *> (__p); __func) {
                assert_global(__func,__ans,__map);
                __func->space = new scope {.prev = __ans};
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

    /* This is used to help deallocate memory */
    wrapper create_function(function *__func) {
        static size_t i = 0;
        return wrapper {
            .type = class_map[std::to_string(i++)] = new typeinfo {__func,""},
            .info = 0,  // No dimension
            .flag = 0   // Not assignable
        };
    }

    /* Get the type wrapper of the identifier. */
    wrapper get_wrapper(identifier *ctx) {
        auto *__p = dynamic_cast <function *> (ctx);
        return __p ? create_function(__p) : ctx->type;
    }

    /* Get the type wrapper of the type by its name. */
    wrapper get_wrapper(std::string ctx) {
        return wrapper {
            .type = class_map[ctx],
            .info = 0,
            .flag = 0
        };
    }


    /* Find the class info within. */
    typeinfo *find_class(const std::string &str) {
        auto iter = class_map.find(str);
        return iter == class_map.end() ? nullptr : iter->second;
    }

    /* Tries to init from ASTbuilder. */
    ASTvisitor(std::vector <definition *>        &__def,
               std::map <std::string,typeinfo *> &__map) {
        std::cout << "\n\n|---------------Start scanning---------------|\n" << std::endl;
        class_map =    ASTClassScanner::scan(__def,__map);
        global    = ASTFunctionScanner::scan(__def,class_map);

        for(auto __p : __def) {
            top = global; /* Current setting. */
            visit(__p);
        }

        std::cout << "No error found.";

        std::cout << "\n\n|----------------End scanning----------------|\n" << std::endl;
    }

    /* Top scope pointer. */
    scope *top = nullptr;

    /* Top type pointer. */
    wrapper current_type;

    /* Top loop pointer. */
    std::vector <loop_type *> loop;

    /* Top function pointer. */
    std::vector <function  *> func;
    
    /* Whether source type is convertible to target type. */
    static bool is_convertible(const wrapper &src,std::string_view __name,int __dim) {
        if(src.check(__name,__dim)) return true;
        if(src.name() == "null") {
            // Array case: ok.
            if(__dim > 0) return true;
            // Not convertible to basic types.
            if(__name == "int"  || 
               __name == "bool" ||
               __name == "string")
                return false;
            // None basic type case: ok.
            return true;    
        } return false;
    }


    /* Whether source type is convertible to target type. */
    static bool is_convertible(const wrapper &src,const wrapper &dst) {
        if(src == dst) return true;
        // There is only one construction function now
        else if(src.name() == "null") {
            // Array case: ok.
            if(dst.dimension() > 0) return true;
            // Null is not convertible to basic types.
            if(dst.name() == "int"  ||
               dst.name() == "bool" ||
               dst.name() == "string")
                return false;
            // None basic type case: ok.
            return true;    
        } else return false;
    }


    void visit(node *ctx) { return ctx->accept(this); }
    void visitBracketExpr(bracket_expr *) override;
    void visitSubscriptExpr(subscript_expr *) override;
    void visitFunctionExpr(function_expr *) override;
    void visitUnaryExpr(unary_expr *) override;
    void visitMemberExpr(member_expr *) override;
    void visitConstructExpr(construct_expr *) override;
    void visitBinaryExpr(binary_expr *) override;
    void visitConditionExpr(condition_expr *) override;
    void visitAtomExpr(atom_expr *) override;
    void visitLiteralConstant(literal_constant *) override;
    void visitForStmt(for_stmt *) override;
    void visitFlowStmt(flow_stmt *) override;
    void visitWhileStmt(while_stmt *) override;
    void visitBlockStmt(block_stmt *) override;
    void visitBranchStmt(branch_stmt *) override;
    void visitSimpleStmt(simple_stmt *) override;
    void visitVariable(variable_def *) override;
    void visitFunction(function_def *) override;
    void visitClass(class_def *) override;

};

}
    