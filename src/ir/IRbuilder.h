#pragma once

#include "utility.h"
#include "IRnode.h"

namespace dark::IR {


struct IRbuilder : AST::ASTvisitorbase {
    /* Global null constant shared by all members. */
    inline static auto *__null__ = new pointer_constant {nullptr};
    inline static auto *__neg1__ = new integer_constant {-1};   /* Literal -1.  */
    inline static auto *__zero__ = new integer_constant {0};    /* Literal 0.   */
    inline static auto *__one1__ = new integer_constant {1};    /* Literal 1.   */
    inline static auto *__true__ = new boolean_constant {true}; /* Literal true. */
    inline static auto *__false__= new boolean_constant {false};/* Literal false. */

    struct flow_type {
        block_stmt *bk; /* Break. */
        block_stmt *ct; /* Continue. */

        /* If true return 'continue'. Else return 'break'. */
        block_stmt *operator [](bool __n) const noexcept 
        { if(__n) return ct; else return bk;}
    };

    std::map <std::string      ,IR::typeinfo *>    class_map;
    std::map <AST::identifier *,IR::function *> function_map;
    std::map <AST::identifier *,IR::variable *> variable_map;

    std::vector <initialization> global_variable;
    std::vector <  function   >  global_function;
    std::vector <  function   >  builtin_function;
    std::vector <  flow_type  >  loop_flow; /* Flow position. */

    scope *global_scope = nullptr;
    function    *top    = nullptr;  /* Current function.*/
    definition  *result = nullptr;  /* Last definition used. */
    size_t function_cnt = 0;        /* Count of functions. */

    /* This will transform AST wrapper to IR wrapper. */
    wrapper get_type(AST::wrapper type) {
        auto *__ptr = class_map[type.name()];
        return { __ptr ,(ssize_t) type.dimension() + !__ptr->is_trivial()};
    }

    IRbuilder(scope *__global,
              std::map <std::string,AST::typeinfo> &__map,
              std::vector <AST::definition *>      &__def)
    : global_scope(__global) {
        make_basic(__map["string"].space,__map["__array__"].space);

        /* Build up user-defined class information. */
        for(auto __p : __def) {
            if(auto *__class = dynamic_cast <AST::class_def *> (__p))
                visitGlobalClass(__class);
            else  if(auto *__func = dynamic_cast <AST::function_def *> (__p))
                ++function_cnt;
        }

        /* This is used to avoid allocation! */
        global_function.reserve(function_cnt);

        /* Collect all member function and global functions. */
        for(auto __p : __def) {
            if(auto *__class = dynamic_cast <AST::class_def *> (__p)) {
                visitGlobalClass(__class);
            } else  if(auto *__func = dynamic_cast <AST::function_def *> (__p)) {
                visitGlobalFunction(__func);
            } else { visitGlobalVariable(safe_cast <AST::variable_def *> (__p)); } 
        }

        /* Visit it! Right now! */
        for(auto __p : __def) { top = nullptr; visit(__p); }

        for(auto &&__class : class_map) {
            if(auto *__ptr = dynamic_cast <class_type *> (__class.second)) {
                std::cout << __ptr->data() << '\n';
            }
        } std::cout << '\n';

        for(auto &__var : global_variable)
            std::cout << __var.data() << '\n';
        std::cout << '\n';


        for(auto &__func : global_function)
            std::cout << __func.data() << '\n';

    }

    void make_basic(scope *__string,scope *__array);

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
    void visitGlobalClass   (AST::class_def *);
    void visitGlobalVariable(AST::variable_def *);
    void visitGlobalFunction(AST::function_def *);
    void visitGlobalVariable(AST::variable *,AST::literal_constant *);
    store_stmt *visitFunctionParam(AST::identifier *);

    /* Return the 'this' pointer. */
    definition *get_this_pointer() {
        // /* If this pointer is pre-loaded! */
        // return top->front_temporary();

        /* If non pre-loaded, loaded it now! */
        result = safe_cast <allocate_stmt *> (top->stmt[0]->stmt[0])->dest;
        return safe_load_variable();
    }

    /* Load a variable from result and return the temporary. */
    temporary *safe_load_variable() {
        auto *__ptr  = safe_cast <non_literal *> (result);
        auto *__load = new load_stmt;
        __load->src  = __ptr;
        __load->dst  = top->create_temporary(__ptr->get_point_type());
        top->emplace_new(__load);
        return __load->dst;
    }

    function *get_string_add() { return &builtin_function[12]; }
    function *get_string_cmp() { return &builtin_function[13]; }

    /* This will only be used in member access. */
    class_type *get_class(const std::string &__name) {
        return safe_cast <class_type *> (class_map[__name]);
    }

    /* Get a temporary name from its index. */
    static std::string get_temporary_name(size_t __n)
    { return "%" + std::to_string(__n); }

    /* Create a string constant and return the variable to it. */
    variable *create_string(const std::string &__name) {
        static size_t __cnt = 0;
        static std::map <std::string,variable *> __map;
        auto [__iter,__result] = __map.insert({__name,nullptr});
        if(!__result) return __iter->second;

        auto *__str = new string_constant {__name};

        /* This is a special variable! */
        auto *__var = new variable;
        __var->name = "@str." + std::to_string(__cnt++);
        __var->type = {class_map["string"],1};

        __iter->second = __var;
        global_variable.push_back({__var,__str});
        return __var;
    }

};

}
