#pragma once

#include "utility.h"
#include "IRnode.h"

namespace dark::IR {


struct IRbuilder : AST::ASTvisitorbase {
    scope *global_scope = nullptr;

    struct flow_type {
        block_stmt *bk; /* Break. */
        block_stmt *ct; /* Continue. */

        /* If true return 'continue'. Else return 'break'. */
        block_stmt *operator [](bool __n) const noexcept 
        { if(__n) return ct; else return bk;}
    };

    std::map <std::string      ,IR::class_type>    class_map;
    std::map <AST::identifier *,IR::function *> function_map;
    std::map <AST::identifier *,IR::variable *> variable_map;

    std::vector <initialization> global_variable;
    std::vector <  function   >  global_function;
    std::vector <  function   >  builtin_function;
    std::vector <  flow_type  >  loop_flow; /* Flow position. */



    function    *top    = nullptr; /* Current function.*/
    definition  *result = nullptr; /* Last definition used. */

    /* This will transform AST type to real type. */
    static IR::typeinfo get_type(AST::typeinfo *type) {
        if(type->name == "int")  return IR::typeinfo::I32;
        if(type->name == "bool") return IR::typeinfo::I1;
        if(type->name == "void") return IR::typeinfo::VOID;
        return IR::typeinfo::PTR;
    }


    /* This will transform AST type to real type. */
    static IR::typeinfo get_type(AST::wrapper &type)
    { return get_type(type.type); }


    IRbuilder(scope *__global,
              std::map <std::string,AST::typeinfo> &__map,
              std::vector <AST::definition *>      &__def)
    : global_scope(__global) {
        make_basic(__map["string"].space,__map["__array__"].space);

        for(auto &&[__name,__id] : __global->data) {
            /* Global variable case. */
            auto *__var = dynamic_cast <AST::variable *> (__id);
            if ( !__var ) continue;
            createGlobalVariable(__var);
        }

        /* Collect all member function and global functions. */
        for(auto __p : __def) {
            if(auto *__class = dynamic_cast <AST::class_def *> (__p)) {
                createGlobalClass(__class);
            } else  if(auto *__func = dynamic_cast <AST::function *> (__p)) {
                createGlobalFunction(__func);
            } else { /* Dothing. */ } 
        }

        /* Visit it! Right now! */
        for(auto __p : __def) { top = nullptr; visit(__p); }

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

    void visitGlobalVariable(AST::variable_def *);

    void createGlobalVariable(AST::variable  *);
    void createGlobalFunction(AST::function  *);
    void createGlobalClass   (AST::class_def *);

    /* Create a string constant and return the variable to it. */
    variable *createString(const std::string &__name) {
        static size_t __cnt = 0;
        static std::map <std::string,variable *> __map;
        auto [__iter,__result] = __map.insert({__name,nullptr});
        if(!__result) return __iter->second;

        auto *__str = new string_constant;
        __str->context = __name;

        auto *__var = new variable;
        __var->name = "@str-" + std::to_string(++__cnt);
        __var->type = typeinfo::PTR;

        __iter->second = __var;
        global_variable.push_back({__var,__str});
        return __var;
    }

    // struct __builtin__info__ {
    //     enum {
    //         array_size      = 0,
    //         string_length   = 1,
    //         string_substr   = 2,
    //         string_parseInt = 3,
    //         string_ord      = 4,
    //         print           = 5,
    //         println         = 6,
    //         printInt        = 7,
    //         printlnInt      = 8,
    //         getString       = 9,
    //         getInt          = 10,
    //         toString        = 11,
    //         string_add      = 12,
    //         string_cmp      = 13,
    //     };
    // };

    function *get_string_add() { return &builtin_function[12]; }
    function *get_string_cmp() { return &builtin_function[13]; }

    void make_basic(scope *__string,scope *__array) {
        /**
         * __array__::size(this)                = 0
         * string::length(this)                 = 1
         * string::substring(this,int l,int r)  = 2
         * string::parseInt(this)               = 3
         * string::ord(this,int n)              = 4
         * ::print(string str)                  = 5
         * ::println(string str)                = 6
         * ::printInt(int n)                    = 7
         * ::printlnInt(int n)                  = 8
         * ::getString()                        = 9
         * ::getInt()                           = 10
         * ::toString(int n)                    = 11
         * ::__string__add__(string a,string b) = 12
         * ::__string__cmp__(string a,string b) = 13
         * 
        */
        builtin_function.resize(14);

        builtin_function[0].type = typeinfo::I32;
        builtin_function[1].type = typeinfo::I32;
        builtin_function[2].type = typeinfo::PTR;
        builtin_function[3].type = typeinfo::I32;
        builtin_function[4].type = typeinfo::I32;
        builtin_function[5].type = typeinfo::VOID;
        builtin_function[6].type = typeinfo::VOID;
        builtin_function[7].type = typeinfo::VOID;
        builtin_function[8].type = typeinfo::VOID;
        builtin_function[9].type = typeinfo::PTR;
        builtin_function[10].type = typeinfo::I32;
        builtin_function[11].type = typeinfo::PTR;
        builtin_function[12].type = typeinfo::PTR;
        builtin_function[13].type = typeinfo::I32;

        builtin_function[0].name = "__array__::size";
        builtin_function[1].name = "string::length";
        builtin_function[2].name = "string::substring";
        builtin_function[3].name = "string::parseInt";
        builtin_function[4].name = "string::ord";
        builtin_function[5].name = "::print";
        builtin_function[6].name = "::println";
        builtin_function[7].name = "::printInt";
        builtin_function[8].name = "::printlnInt";
        builtin_function[9].name = "::getString";
        builtin_function[10].name = "::getInt";
        builtin_function[11].name = "::toString";
        builtin_function[12].name = "::__string__add__";
        builtin_function[13].name = "::__string__cmp__";

        auto *__builtin = builtin_function.data();
        function_map[__array->find("size")]             = __builtin + 0;
        function_map[__string->find("length")]          = __builtin + 1;
        function_map[__string->find("substring")]       = __builtin + 2;
        function_map[__string->find("parseInt")]        = __builtin + 3;
        function_map[__string->find("ord")]             = __builtin + 4;
        function_map[global_scope->find("print")]       = __builtin + 5;
        function_map[global_scope->find("println")]     = __builtin + 6;
        function_map[global_scope->find("printInt")]    = __builtin + 7;
        function_map[global_scope->find("printlnInt")]  = __builtin + 8;
        function_map[global_scope->find("getString")]   = __builtin + 9;
        function_map[global_scope->find("getInt")]      = __builtin + 10;
        function_map[global_scope->find("toString")]    = __builtin + 11;
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
