#include "mx.h"
#include "IRbasic.h"


namespace dark::OPT {

/**
 * Special optimizations tailored for languange Mx.
 * 
 * e.g.:
 * print(toString(%number)) -> printInt(%number)
 * parseInt(toString(%number)) -> %number
 * 
 * %array = new_array (%size)
 * ...
 * %x = call __array_size %array -> %x = %size
 * 
 * 
 */
special_judger::special_judger
    (IR::function *__func, node *__entry,void *__this) {
    if (__func->is_unreachable()) return;
    entry_block = __func->stmt.front(); 
    set_impl_ptr(__this);
    collect_use(__func);

    for(auto __block : __func->stmt) {
        top_block = __block;
        for(auto __node : __block->stmt)
            visit(__node);
    }

    replace_function(__func);
    replace_definition(__func);

}

void special_judger::collect_use(IR::function *__func) {
    for(auto __block : __func->stmt) {
        for(auto __node : __block->stmt) {
            auto *__def = __node->get_def();
            if (__def) {
                auto &__ref = use_map[__def];
                __ref.def_node  = __node;
                __ref.def_block = __block;
            }
            for(auto __use : __node->get_use()) {
                auto __tmp = dynamic_cast <IR::temporary *> (__use);
                if (!__tmp) continue;
                use_map[__tmp].use_count++;
            }
        }
    }
}


void special_judger::visitCall(IR::call_stmt *__stmt) {
    auto *__func = __stmt->func;
    if (!__func->is_builtin) return;

    /* Judge whether it is the result of toString and return the arglist. */
    auto &&__is_call_result = [this](IR::definition *__def)
        -> IR::call_stmt * {
        auto *__tmp =  dynamic_cast <IR::temporary *> (__def);
        if (!__tmp) return nullptr;
        const auto &__ref = use_map.at(__tmp);
        auto *__call = dynamic_cast <IR::call_stmt *> (__ref.def_node);
        if (!__call || !__call->func->is_builtin) return nullptr;
        return __call;
    };

    auto &&__check_builtin = [](IR::call_stmt *__call,size_t __target)
        -> IR::definition ** {
        return /* Call target function. */
            __call && IR::IRbasic::get_builtin_index(__call->func) == __target
                ? __call->args.data() : nullptr;
    };

    auto __cur = IR::IRbasic::get_builtin_index(__func);
    if (__cur == 5 || __cur == 6) {
        // print(toString())   --> printInt()
        // println(toString()) --> printlnInt()
        auto __call = __is_call_result(__stmt->args[0]);
        auto __args = __check_builtin(__call,11);
        if (!__args) return;

        auto *__new = new IR::call_stmt;
        __new->func = IR::IRbasic::builtin_function.data() + __cur + 2;
        __new->args.push_back(__args[0]);
        call_map[__stmt] = __new;
    } else if (__cur == 3) {
        // parseInt(toString(%num)) --> %num
        auto __call = __is_call_result(__stmt->args[0]);
        auto __args = __check_builtin(__call,11);
        if (__args) val_map[__stmt->dest] = __args[0];
    } else if (__cur == 0) {
        // sizeof new_array(%len) --> %len
        auto __call = __is_call_result(__stmt->args[0]);
        auto __args = __check_builtin(__call,19);
        if (!__args) __args = __check_builtin(__call,20);
        if (__args) {
            // Do not enable in every case
            if ((dynamic_cast <IR::literal *> (__args[0]))
             || (dynamic_cast <IR::function_argument *> (__args[0])
              && top_block == entry_block))
                val_map[__stmt->dest] = __args[0];
        }
    }
}


void special_judger::visitBlock(IR::block_stmt *) {}
void special_judger::visitFunction(IR::function *) {}
void special_judger::visitInit(IR::initialization *) {}
void special_judger::visitCompare(IR::compare_stmt *) {}
void special_judger::visitBinary(IR::binary_stmt *) {}
void special_judger::visitJump(IR::jump_stmt *) {}
void special_judger::visitBranch(IR::branch_stmt *) {}
void special_judger::visitLoad(IR::load_stmt *) {}
void special_judger::visitStore(IR::store_stmt *) {}
void special_judger::visitReturn(IR::return_stmt *) {}
void special_judger::visitAlloc(IR::allocate_stmt *) {}
void special_judger::visitGet(IR::get_stmt *) {}
void special_judger::visitPhi(IR::phi_stmt *) {}
void special_judger::visitUnreachable(IR::unreachable_stmt *) {}


void special_judger::replace_function(IR::function *__func) {
    for(auto __block : __func->stmt) {
        for(auto &__node : __block->stmt) {
            auto __call = dynamic_cast <IR::call_stmt *> (__node);
            if (!__call) continue;
            auto __iter = call_map.find(__call);
            if (__iter != call_map.end()) {
                delete __call;
                __node = __iter->second;
            }
        }
    }
}

void special_judger::replace_definition(IR::function *__func) {
    runtime_assert("Die!" , !val_map.count(nullptr));

    /* Pathway compression. */
    auto &&__compress = [this] (auto &&__self,IR::definition *__new)
        -> IR::definition * {
        auto *__tmp = dynamic_cast <IR::temporary *> (__new);
        auto __iter = val_map.find(__tmp);
        if (__iter == val_map.end()) return __new;
        return __new = __self(__self,__iter->second);
    };

    /* First, compress the definition chain. */
    for(auto &[__old,__new] : val_map)
        __new = __compress(__compress,__new);

    /* Then, replace all the definitions. */
    for(auto __block : __func->stmt) {
        for(auto __stmt : __block->stmt) {
            for(auto __use : __stmt->get_use()) {
                auto *__tmp = dynamic_cast <IR::temporary *> (__use);
                auto __iter = val_map.find(__tmp);
                if (__iter == val_map.end()) continue;
                __stmt->update(__use,__iter->second);
            }
        }
    }
}


}