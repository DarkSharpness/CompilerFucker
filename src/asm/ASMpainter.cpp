#include "ASMallocator.h"


namespace dark::ASM {

/* After allocation, try to coallesce the registers. */
void ASMallocator::paint_color(function *__func) {
    /// TODO:  Coallesce the registers.
    /* Simplest coloring. */

    callee_save = {
        get_physical(8), // s0
        get_physical(9), // s1
        get_physical(18),// s2
        get_physical(19),// s3
        get_physical(20),// s4
        get_physical(21),// s5
        get_physical(22),// s6
        get_physical(23),// s7
        get_physical(24),// s8
        get_physical(25),// s9
        get_physical(26),// s10
        get_physical(27),// s11
        get_physical(3), // gp
        get_physical(4), // tp
    };

    caller_save = {
        get_physical(10),// a0
        get_physical(11),// a1
        get_physical(12),// a2
        get_physical(13),// a3
        get_physical(14),// a4
        get_physical(15),// a5
        get_physical(16),// a6
        get_physical(17),// a7
        get_physical(7), // t2
        get_physical(28),// t3
        get_physical(29),// t4
        get_physical(30),// t5
        get_physical(31),// t6
        get_physical(2)  // ra
    };

    /* Resolve caller save memory. */
    for(auto &&[__reg,__ref] : vir_map) {
        if (__ref.type == address_info::CALLER) {
            auto &__ptr = __impl->usage_map.at(__reg);
            for(auto [__call,__idx] : __ptr.save_set) {
                size_t i = 0;
                for(; i < stack_map.size() ; ++i) {
                    auto &__slot = stack_map[i];
                    /* Can not put into current stack. */
                    if (!__slot.contains(__idx - 1,__idx + 1)) continue;
                    /* Can put into current slot! */
                    __slot.live_range.insert({__idx - 1,__idx + 1});
                    __call->caller_save.push_back(
                        { caller_save[__ref.index] , i }
                    ); break;
                }

                /* Not assigned. */
                if (i == stack_map.size()) {
                    stack_map.emplace_back().
                        live_range.insert({__idx - 1,__idx + 1});
                    __call->caller_save.push_back(
                        { caller_save[__ref.index] , i }
                    );
                }
            }
        }
    }

    /* Make the entry. */
    for(auto &__addr : __func->dummy_def) {
        auto &__reg = __addr.pointer.reg;
        __reg = resolve_def(__reg);
        if (expr_list.empty()) continue;
        auto __store = safe_cast <store_memory *> (expr_list.back());
        expr_list.pop_back();
        __addr = __store->addr;
    }

    /* Color the graph. */
    for(auto __block : __func->blocks) {
        for(auto __node : __block->expression) visit(__node);
        expr_list.swap(__block->expression);
        expr_list.clear();
    }
    
    __func->stk_count = stack_map.size();
}

void ASMallocator::visitArithReg(arith_register *__ptr) {
    /* No temporaries are needed. */
    __ptr->lval = resolve_use(__ptr->lval);
    __ptr->rval = resolve_use(__ptr->rval);
    expr_list.push_back(__ptr);
    __ptr->dest = resolve_def(__ptr->dest);
}

void ASMallocator::visitArithImm(arith_immediat *__ptr) {
    __ptr->lval = resolve_use(__ptr->lval);
    expr_list.push_back(__ptr);
    __ptr->dest = resolve_def(__ptr->dest);
}

void ASMallocator::visitMoveReg(move_register *__ptr) {
    size_t __i = expr_list.size();
    auto __use = resolve_use(__ptr->from);
    bool   __j = expr_list.size() > __i;

    /* If temporary value, it should not be counted. */
    __j = __j && dynamic_cast <load_memory *> (expr_list.back());
    expr_list.push_back(__ptr);

           __i = expr_list.size();
    auto __def = resolve_def(__ptr->dest);
    bool   __k = expr_list.size() > __i;

    switch (__k << 1 | __j) {
        case 0: // reg->reg
            __ptr->from = __use;
            __ptr->dest = __def;
            return;

        case 1: // mem->reg
            safe_cast <load_memory *>
                (*(expr_list.end() - 2))->dest = __def;
            expr_list.pop_back();
            return;

        case 2: // reg->mem
            safe_cast <store_memory *>
                (*(expr_list.end () - 2) = expr_list.back())
                    ->from = __use;
            expr_list.pop_back();
            return;
    }

    /* Special case: mem->mem. */
    auto __load  = safe_cast <load_memory *>  (*(expr_list.end () - 3));
    auto __store = safe_cast <store_memory *> (*(expr_list.end () - 1));
    /* Assign the same address: no movement. */
    if (__load->addr.stack.index == __store->addr.stack.index)
        expr_list.pop_back() , expr_list.pop_back() , expr_list.pop_back();
    else {
        __load->dest = __store->from = __use;
        *(expr_list.end () - 2) = expr_list.back();
        expr_list.pop_back();
    }
}

void ASMallocator::visitSltReg(slt_register *__ptr) {
    __ptr->lval = resolve_use(__ptr->lval);
    __ptr->rval = resolve_use(__ptr->rval);
    expr_list.push_back(__ptr);
    __ptr->dest = resolve_def(__ptr->dest);
}

void ASMallocator::visitSltImm(slt_immediat *__ptr) {
    __ptr->lval = resolve_use(__ptr->lval);
    expr_list.push_back(__ptr);
    __ptr->dest = resolve_def(__ptr->dest);
}

void ASMallocator::visitBoolNot(bool_not *__ptr) {
    __ptr->from = resolve_use(__ptr->from);
    expr_list.push_back(__ptr);
    __ptr->dest = resolve_def(__ptr->dest);
}

void ASMallocator::visitBoolConv(bool_convert *__ptr) {
    __ptr->from = resolve_use(__ptr->from);
    expr_list.push_back(__ptr);
    __ptr->dest = resolve_def(__ptr->dest);
}

void ASMallocator::visitLoadSym(load_symbol *__ptr) {
    expr_list.push_back(__ptr);
    __ptr->dest = resolve_def(__ptr->dest);
}

void ASMallocator::visitLoadMem(load_memory *__ptr) {
    if (__ptr->addr.type == value_type::POINTER) {
        auto &__reg = __ptr->addr.pointer.reg;
        __reg = resolve_use(__reg);
    }
    expr_list.push_back(__ptr);
    __ptr->dest = resolve_def(__ptr->dest);
}

void ASMallocator::visitStoreMem(store_memory *__ptr) {
    __ptr->from = resolve_use(__ptr->from);
    if (__ptr->addr.type == value_type::POINTER) {
        auto &__reg = __ptr->addr.pointer.reg;
        __reg = resolve_use(__reg);
    } else { /* GLOBAL */
        auto &__reg = __ptr->addr.global.reg;
        __reg = find_temporary();
    }
    expr_list.push_back(__ptr);
}

void ASMallocator::visitCallFunc(function_node *__ptr) {
    /* Deal with its 8 arguments. */
    expr_list.push_back(__ptr);
    for(auto &__addr : __ptr->dummy_use) {
        /* No need to do anything if global. */
        if (__addr.type == value_type::GLOBAL) continue;

        auto &__reg = __addr.pointer.reg;

        /* Constant pointer value case: load it later. */
        if (auto __tmp = dynamic_cast <physical_register *> (__reg)) continue;

        if (__addr.pointer.offset != 0) runtime_assert("No address shall be taken!");
        auto __vir = safe_cast <virtual_register *> (__reg);

        auto __tmp = vir_map.at(__vir);
        switch(__tmp.type) {
            case address_info::CALLEE:
                __reg = callee_save[__tmp.index]; break;
            case address_info::CALLER:
                __reg = caller_save[__tmp.index]; break;
            case address_info::TEMPORARY:
                __reg = get_physical(__tmp.index); break;
            default: // In stack: change this to stack_address.
                __addr = stack_address {top_func,__tmp.index};
        }
    }

    /* If no need to resolve, then do nothing. */
    if (__ptr->dest) {
        size_t __i = expr_list.size();
        __ptr->dest = resolve_def(__ptr->dest);
        if (expr_list.size() > __i) {
            /* A new store memory is inserted: dst spilled. */
            auto __store  = safe_cast <store_memory *> (expr_list.back());
            expr_list.pop_back();
            __ptr->is_dest_spilled = true;
            __ptr->spilled_offset  = __store->addr.stack.index;
        }
    }
}


void ASMallocator::visitRetExpr(ret_expression *__ptr) {
    if (!__ptr->rval) return void(expr_list.push_back(__ptr));
    size_t __i = expr_list.size();
    __ptr->rval = resolve_use(__ptr->rval);
    if (expr_list.size() > __i) {
        if (auto __load = dynamic_cast <load_memory *> (expr_list.back())) {
            __load->dest = get_physical(10);
            expr_list.pop_back();
        } else {
            auto __arith = safe_cast <arith_immediat *> (expr_list.back());
            __arith->dest = get_physical(10);
        } __ptr->rval  = nullptr;
    }
    expr_list.push_back(__ptr);
}

void ASMallocator::visitJumpExpr(jump_expression *__ptr) {
    expr_list.push_back(__ptr);
}

void ASMallocator::visitBranchExpr(branch_expression *__ptr) {
    __ptr->lvar = resolve_use(__ptr->lvar);
    __ptr->rvar = resolve_use(__ptr->rvar);
    expr_list.push_back(__ptr);
}






}