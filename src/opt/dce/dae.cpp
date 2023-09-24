#include "deadcode.h"



namespace dark::OPT {

dead_argument_eliminater::dead_argument_eliminater
    (IR::function *__func, node *) {
    auto *__zero__ = IR::create_integer(0);
    auto *__null__ = IR::create_pointer(nullptr);
    auto *__false_ = IR::create_boolean(false);
    for(auto __block : __func->stmt)
        for(auto __node : __block->stmt) {
            auto __call = dynamic_cast <IR::call_stmt *> (__node);
            if (!__call) continue;
            for(size_t i = 0 ; i != __call->args.size() ; ++i) {
                if (__call->func->args[i]->is_dead()) {
                    auto &__arg = __call->args[i];
                    auto __name = __arg->get_value_type().name();
                    if (__name == "i32")     __arg = __zero__;
                    else if (__name == "i1") __arg = __false_;
                    else __arg = __null__;
                }
            }
        }
}

}