#pragma once

#include "ASTscanner.h"


namespace dark::AST {


struct ASTvisitor : ASTvisitorbase {
    /* Global scope. */
    scope *global = nullptr;
    /* Global mapping from name to specific type. */

    /* From a class to its specific info. */
    std::map <std::string,typeinfo> class_map;
    /* From a function to its wrapper. */
    std::map <function * ,typeinfo> function_map;

    function_def *global_init; /* This is the special global init function. */

    /* This is used to help deallocate memory */
    wrapper create_function(function *__func) {
        /* Return temporary data location. */
        auto [__iter,__] = function_map.insert(
            {__func,typeinfo {.func = __func}}
        );
        return wrapper {
            .type = &__iter->second,
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
            .type = &class_map[ctx],
            .info = 0,
            .flag = 0
        };
    }

    /* Find the class info within. */
    typeinfo *find_class(const std::string &str) {
        auto iter = class_map.find(str);
        return iter == class_map.end() ? nullptr : &iter->second;
    }

    /* Tries to init from ASTbuilder. */
    ASTvisitor(std::vector <definition *>      &__def,
               std::map <std::string,typeinfo> &__map) {
        std::cout << "\n\n|---------------Start scanning---------------|\n" << std::endl;
        class_map =    ASTClassScanner::scan(__def,__map);
        global    = ASTFunctionScanner::scan(__def,class_map);

        /* A never called global function. */
        global_init = new function_def;
        global_init->space = new scope{.prev = global};
        global_init->body  = new block_stmt;
        global_init->name  = global_init->unique_name = "::__global__init__";
        global_init->type  = get_wrapper("void");

        for(auto __p : __def) {
            top = global; /* Current setting. */
            visit(__p);
        }

        assert_main();

        __def.push_back(global_init);

        /* Pass the semantic check! */
        std::cout << "No error is found. Semantic check pass!";

        // for(auto __p : __def) {
        //     if(auto *__func = dynamic_cast <function *> (__p)) {
        //         std::cout << '\n';
        //         for(auto &&__iter : __func->args)
        //             std::cout << __func->space->find(__iter.name)->unique_name 
        //                       << ' ';
        //         for(auto __iter : __func->unique_mapping)
        //             std::cout << __iter->unique_name << ' ';
        //     } else if(auto *__var = dynamic_cast <variable_def *> (__p)) {
        //         std::cout << '\n';
        //         for(auto &&__iter : __var->init)
        //             std::cout << global->find(__iter.first)->unique_name
        //                       << ' ';
        //     }
        // }

        // std::cout << '\n'; global_init->print();

        std::cout << "\n\n|----------------End scanning----------------|\n" << std::endl;
    }

    /* Top scope pointer. */
    scope *top = nullptr;

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

    /* Return the unique name of the variable. */
    static std::string get_unique_name(variable *__var,function *__func) {
        /* Global variable: no renaming. */
        if(!__func) return '@' + __var->name;
        else { /* Function case. */
            /* Every suffix in one function will never be identical. */
            static std::map <function *,size_t> __cnt;

            /* The line below is removed because function arguments not in alloca! */

            return '%' + __var->name + '-' + std::to_string(__cnt[__func]++);
        }
    }

    /**
     * @brief Check main function and add return 0 to it.
     * @throw dark::error
     * 
    */
    void assert_main() {
        auto *__func = dynamic_cast <function *> (global->find("main"));
        if(!__func || !__func->args.empty() || !__func->type.check("int",0))
            throw error("No valid main function!");

        /* Add return 0 to main function. */
        auto *__tail = new flow_stmt;
        auto *__zero = new literal_constant;
        __zero->name = "0";
        __zero->type = AST::literal_constant::NUMBER;
        __tail->flow = "return";
        __tail->expr = __zero;
        __func->body->stmt.push_back(__tail);
        __zero->space = __tail->space = __func->space;
    }


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
