#include "ASMbuilder.h"


namespace dark::ASM {

function_node *ASMbuilder::builtin_inline(IR::call_stmt *__stmt,Register *__dest) {
    size_t __index = get_builtin_index(__stmt->func);
    switch(__index) {
        case 0: /* __Array_size__   */
        if (tail_call_set.count(__stmt)) __dest = get_physical(10);
        return [this,__stmt,__dest]() -> std::nullptr_t {
            auto __val = get_value(__stmt->args[0]);
            runtime_assert("",__val.type == __val.POINTER);
            top_block->emplace_back(new load_memory {
                load_memory::WORD, __dest,
                __val.pointer += (-4)
            });
            /* Tail call case. */
            if (tail_call_set.count(__stmt))
                top_block->emplace_back(
                    new ret_expression {top_asm,__dest}
                );
            return nullptr;
        } ();

        case 4: /* __String_ord__   */
        if (tail_call_set.count(__stmt)) __dest = get_physical(10);
        return [this,__stmt,__dest]() -> std::nullptr_t {
            auto *__ptr = __stmt->args[0];
            auto *__idx = __stmt->args[1];
            if (auto __lit = dynamic_cast <IR::integer_constant *> (__idx)) {
                auto __val = get_value(__ptr);
                runtime_assert("",__val.type == __val.POINTER);
                top_block->emplace_back(new load_memory {
                    load_memory::BYTE, __dest,
                    __val.pointer += __lit->value
                });
            } else {
                auto *__vir = create_virtual();
                top_block->emplace_back(new arith_register {
                    arith_base::ADD,
                    force_register(__idx),
                    force_register(__ptr), __vir
                });
                top_block->emplace_back(new load_memory {
                    load_memory::BYTE, __dest,
                    pointer_address {__vir,0}
                });
            }
            /* Tail call case. */
            if (tail_call_set.count(__stmt))
                top_block->emplace_back(
                    new ret_expression {top_asm,__dest}
                );
            return nullptr;
        } ();

        case 3:  /* __String_parseInt__ */
        case 5:  /* __print__           */
        case 7:  /* __printInt__        */
        case 8:  /* __printlnInt__      */
        case 9:  /* __getString__       */
        case 10: /* __getInt__          */
        case 11: /* __toString__        */
        case 12: /* __string_add__      */
        case 19: /* __new_array1__      */
        case 20: /* __new_array2__      */

        /* No inlining case (libc/long function). */
        default: break;
    }

    return new call_function {get_function(__stmt->func),top_asm};
}


void ASMbuilder::visitCall(IR::call_stmt *__stmt) {
    function_node *__call = nullptr;
    Register *__dest = __stmt->dest && use_map[__stmt->dest].count > 0 ?
        get_virtual(__stmt->dest) : nullptr;

    if (__stmt->func->is_builtin) {
        __call = builtin_inline(__stmt,__dest);
        if (!__call) return;
    } else {
        auto *__func = get_function(__stmt->func);
        __call = new call_function {__func,top_asm};
    }

    /* Tail call doesn't require saving ra. */
    if (tail_call_set.count(__stmt)) {
        __call->op   = call_function::TAIL;
        __call->dest = nullptr;
    } else { /* Non-tail call requires saving ra. */
        top_asm->save_ra = true;
        __call->dest = __dest;
    }

    /* Loading arguments into dummy use. */
    for (size_t i = 0 ; i < __stmt->args.size() ; ++i) {
        auto *__arg = __stmt->func->args[i];
        /* Do nothing to dead arguments. */
        if (__arg->is_dead()) continue;

        if (i < 8) { /* Dummy reordering of those arguments. */
            __call->dummy_use.push_back(get_value(__stmt->args[i]));
        } else { /* Excessive arguments stored to stack. */
            top_block->emplace_back(new store_memory {
                memory_base::WORD, force_register(__stmt->args[i]),
                pointer_address{ get_physical(2), ssize_t(i - 8) << 2 }
            });
        }
    }

    top_block->emplace_back(__call);
}



}