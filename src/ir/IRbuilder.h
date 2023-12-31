#pragma once

#include "utility.h"
#include "IRnode.h"

#include "IRbasic.h"

namespace dark::IR {


struct IRbuilder : AST::ASTvisitorbase , IRbasic {
    /* Global null constant shared by all members. */
    inline static auto *__null__ = create_pointer(nullptr);
    inline static auto *__neg1__ = create_integer(-1);   /* Literal -1.  */
    inline static auto *__zero__ = create_integer(0);    /* Literal 0.   */
    inline static auto *__one1__ = create_integer(1);    /* Literal 1.   */
    inline static auto *__true__ = create_boolean(true) ; /* Literal true. */
    inline static auto *__false__= create_boolean(false);/* Literal false. */

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

    std::vector <initialization> global_variables;
    std::vector <  function   >  global_functions;
    std::vector <  flow_type  >  loop_flow; /* Flow position. */

    scope *global_scope = nullptr;
    function    *top    = nullptr;  /* Current function.*/
    definition  *result = nullptr;  /* Last definition used. */
    size_t function_cnt = 0;        /* Count of functions. */

    function *main_function = nullptr; /* Main function. */

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
        global_functions.reserve(function_cnt);

        /* Collect all member function and global functions. */
        for(auto __p : __def) {
            if(auto *__class = dynamic_cast <AST::class_def *> (__p)) {
                visitGlobalClass(__class);
            } else  if(auto *__func = dynamic_cast <AST::function_def *> (__p)) {
                visitGlobalFunction(__func);
            } else { visitGlobalVariable(safe_cast <AST::variable_def *> (__p)); } 
        }

        { /* If init is empty, then it won't be generated. */
            auto *__func = safe_cast <AST::function_def *> (__def.back());
            if(__func->body->stmt.empty()) {
                delete __func;
                global_functions.pop_back();
                __def.pop_back();
            } else { /* Add call statement to global init. */
                auto *__call = new call_stmt;
                __call->dest = nullptr;
                __call->func = function_map[__func];
                main_function->emplace_new(__call);
            }
        }

        /* Visit it! Right now! */
        for(auto __p : __def) { top = nullptr; visit(__p); }

    }

    void debug_print(std::ostream &os) {
        for(auto &&__class : class_map) {
            if(auto *__ptr = dynamic_cast <class_type *> (__class.second)) {
                os << __ptr->data() << '\n';
            }
        } os << '\n';

        for(auto &__var : global_variables)
            os << __var.data() << '\n';
        os << '\n';

        for(auto &__func : global_functions)
            os << __func.data() << '\n';
        os << '\n';

        for(auto &__var : builtin_function)
            os << __var.declare();
        os << '\n';

        for(auto &[__name,__var] : string_map) {
            auto __str = safe_cast <string_constant *> (__var->const_val);
            os << __var->data()     << " = "
               << __str->type_data() << ' ' << __str->data() << '\n';
        }
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
    void visitStringBinary(AST::binary_expr *);
    void visitNewExpr(wrapper,std::vector <definition *>);
    store_stmt *visitFunctionParam(AST::identifier *);


    /* Return the 'this' pointer. */
    definition *get_this_pointer() {
        // /* If this pointer is pre-loaded! */
        // return top->front_temporary();

        /* If non pre-loaded, loaded it now! */
        result = safe_cast <allocate_stmt *> (top->stmt.front()->stmt.front())->dest;
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
    function *get_string_cmp() { return &builtin_function[22]; }
    function *get_string_cmp(decltype(compare_stmt::op) __op)
    { return &builtin_function[13 + __op]; }

    function *get_new_array(size_t __n) {
        if(__n == 4) return &builtin_function[20];
        if(__n == 1) return &builtin_function[19];
        throw error("Invalid array length!");
    }
    function *get_new_object() { return &builtin_function[21]; }

    /* This will only be used in member access. */
    class_type *get_class(const std::string &__name) {
        return safe_cast <class_type *> (class_map[__name]);
    }

    /* Get a temporary name from its index. */
    static std::string get_temporary_name(size_t __n)
    { return "%" + std::to_string(__n); }

    /* Test whether the function is global. */
    static bool is_global_function(std::string_view);

    /* Help to generate better code. */
    string_constant *find_match_string(global_variable *__var) {
        auto __beg = global_variables.crbegin();
        auto __end = global_variables.crend();
        for(; __beg != __end ; ++__beg) {
            auto [__tmp,__lit] = *__beg;
            if (__tmp == __var)
                return safe_cast <string_constant *> (__lit);
        } throw error("No matched string is found");
    }

    /* Create a jump statement. */
    void create_jump(block_stmt *__block) {
        auto *__jump = new jump_stmt;
        __jump->dest = __block;
        top->emplace_new(__jump);
    }

    static std::string function_name_map(const std::string &__str) {
        std::string __ret;
        __ret.reserve(__str.length());
        for(size_t i = 0 ; i < __str.length() ; ++i) {
            if(__str[i] == ':') __ret += '.', ++i;
            else if(__str[i] == '.') throw error("Unexpected error!");
            else __ret += __str[i];
        } return __ret;
    }

};


}
