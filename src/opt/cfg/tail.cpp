#include "cfg.h"

#include "mem2reg.h"

namespace dark::OPT {

tail_recursion_pass::tail_recursion_pass
    (IR::function *__func,void *__SSA) {
    auto *__ptr = static_cast <SSAbuilder *> (__SSA);
    std::vector <IR::block_stmt *> __qwq; /* New block list. */

    for(auto __block : __func->stmt) {
        auto __node = __block->get_impl_ptr <node> ();
        auto &__vec = __block->stmt;
        if (__vec.size() == 2) {
            auto *__phi = dynamic_cast <IR::phi_stmt *>    (__vec[0]);
            auto *__ret = dynamic_cast <IR::return_stmt *> (__vec[1]);
            if (!__phi || !__ret) continue;
            if (__ret->rval != __phi->dest) {
                warning("This shouldn't happen!");
                continue;
            }

            __node->prev.clear();

            /* Relinking! */
            for(auto [__val,__from] : __phi->cond) {
                auto *__prev  = __from->get_impl_ptr <node> ();

                /* Creating new block. */
                auto *__temp  = new IR::block_stmt; 
                __temp->label = __func->create_label("phi-ret");
                __ret = new IR::return_stmt;
                __ret->func   = __func;
                __ret->rval   = __val;
                __temp->stmt  = { __ret };
                __qwq.push_back(__temp);

                /* Create a block. */
                auto *__next  = __ptr->create_node(__temp);
                __next->prev  = {__prev};
                __temp->set_impl_ptr(__next);

                auto *&__last = __prev->block->stmt.back();
                if (auto *__jump = dynamic_cast <IR::jump_stmt *> (__last)) {
                    __jump->dest = __temp;
                    __prev->next[0] = __next;
                } else { /* Branch case now. */
                    auto *__br = safe_cast <IR::branch_stmt *> (__last);
                    branch_compressor::update_branch(__br,__prev,__node,__next);
                }
            }
        }
    }

    __func->stmt.insert(__func->stmt.end(),__qwq.begin(),__qwq.end());
}


}