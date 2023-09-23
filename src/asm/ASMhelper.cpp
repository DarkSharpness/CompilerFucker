#include "ASMbuilder.h"

#include <queue>

namespace dark::ASM {


pointer_address ASMbuilder::get_pointer_address(IR::non_literal *__def) {
    if (!__def) return pointer_address { get_physical(0), 0 };
    else if (auto __local = dynamic_cast <IR::local_variable *> (__def)) {
        return pointer_address { get_physical(2), offset_map.at(__local) };
    } else if (auto __arg = dynamic_cast <IR::function_argument *> (__def)) {
        return pointer_address { get_virtual(__arg),0};
    } else {
        auto __tmp = safe_cast <IR::temporary *> (__def);
        auto __iter = getelement_map.find(__tmp);
        return __iter == getelement_map.end() ?
            pointer_address { get_virtual(__tmp),0} :
            (get_pointer_address(__iter->second.base) += __iter->second.offset);
    }
}


value_type ASMbuilder::get_value(IR::non_literal *__def) {
    if (auto __global = dynamic_cast <IR::global_variable *> (__def))
        return global_address { nullptr, __global };
    else return get_pointer_address(__def);
}


value_type ASMbuilder::get_value(IR::definition *__def) {
    if (!__def) return pointer_address {nullptr,0};
    if (auto __ptr = dynamic_cast <IR::pointer_constant *> (__def);
        __ptr != nullptr && __ptr->var != nullptr)
        return global_address {
            nullptr, const_cast <IR::global_variable *> (__ptr->var)
        };

    if (auto __lit = dynamic_cast <IR::literal *> (__def))
        return pointer_address {
            get_physical(0), __lit->get_constant_value()
        };

    return get_value(safe_cast <IR::non_literal *> (__def));
}


virtual_register *ASMbuilder::create_virtual() {
    return top_asm->create_virtual();
}


virtual_register *ASMbuilder::get_virtual(IR::non_literal *__var) {
    if (auto __temp = dynamic_cast <IR::temporary *> (__var))
        if (getelement_map.count(__temp))
            runtime_assert("Get results shouldn't be passed out!");

    auto &__ref = temp_map[__var];
    return __ref ? __ref : (__ref = create_virtual());
}

Register *ASMbuilder::force_register(IR::definition *__def) {
    auto __use = get_value(__def);
    if (__use.type == value_type::POINTER) {
        if (__use.pointer.offset == 0)
            return __use.pointer.reg;
        else { /* Need to be calculated out. */
            /* Constant value case: Use temporary register. */
            if (auto __reg = dynamic_cast <physical_register *> (__use.pointer.reg))
                return get_constant(__reg,__use.pointer.offset);

            auto *__reg = create_virtual();
            top_block->emplace_back(new arith_immediat {
                arith_base::ADD, __use.pointer.reg,
                __use.pointer.offset, __reg
            }); return __reg;
        }
    } else if (__use.type == value_type::GLOBAL) {
        auto *__reg = create_virtual();
        top_block->emplace_back(new load_symbol {
            load_symbol::FULL, __reg, __use.global.var
        }); return __reg;
    } else throw error("Fuck!");
}


block *ASMbuilder::get_edge(IR::block_stmt *prev,IR::block_stmt *next) {
    if (!dynamic_cast <IR::phi_stmt *> (next->stmt.front()))
        return link(get_block(prev), get_block(next));

    /* Tries to find in existed map. */
    auto &__ref = branch_map[prev];
    if (__ref.key[0] == next) return __ref.val[0];
    if (__ref.key[1] == next) return __ref.val[1];

    /* Build up a new critical edge block. */
    block *__temp = nullptr;
    if (__ref.key[0] == nullptr) {
        __ref.key[0] = next;
        __ref.val[0] = __temp = get_virtual_block <4> (prev);
    } else if (__ref.key[1] == nullptr) {
        __ref.key[1] = next;
        __ref.val[1] = __temp = get_virtual_block <8> (prev);
    } else runtime_assert("WTF ???");

    /* Only expression: jump to the target block after usage. */
    __temp->expression = { new jump_expression {
        link( link(get_block(prev), __temp ), get_block(next))}
    };
    return __temp;
}


void ASMbuilder::resolve_phi(phi_info &__ref) {
    runtime_assert("die",top_block->expression.size() == 1);
    auto *__jump = safe_cast <jump_expression *> (top_block->expression[0]);
    struct node_data {
        virtual_register *in {nullptr};
        value_type       val {nullptr};
        size_t           out    {0};
    };

    std::unordered_map <virtual_register *, node_data> __map;
    __map.reserve(__ref.use.size() << 1 | 1);

    top_block->expression.clear();
    for(size_t i = 0 ; i != __ref.use.size() ; ++i) {
        auto __def = __ref.def[i];
        auto __use = __ref.use[i];
        __map[__def].val = __use;
        if (__use.type == value_type::POINTER) {
            auto *__reg = dynamic_cast <virtual_register *> (__use.pointer.reg);
            if (__reg != nullptr && __reg != __def) {
                __map[__def].in = __reg;
                __map[__reg].out++;
            }
        }
    }

    std::queue <virtual_register *> __work_list;

    for(auto &[__reg,__dat] : __map)
        if (!__dat.out) __work_list.push(__reg);

    /* Leave the cycles behind. */
    while(!__work_list.empty()) {
        auto *__def = __work_list.front(); __work_list.pop();
        auto __iter = __map.find(__def);
        auto *__in  = __iter->second.in;
        auto  __use = __iter->second.val;
        __map.erase(__iter);
        /* No use. */
        if (__use.type == value_type::STACK) continue;
        auto *__node = try_assign(__def,__use);
        /* Assign to itself: Nothing should be done. */
        if (!__node) continue;
        top_block->emplace_back(__node);
        if (__in) { /* Topologically! */
            auto &__dat = __map.at(__in);
            if (!--__dat.out) __work_list.push(__in);
        }
    }

    top_block->emplace_back(__jump);

    if (!__map.empty()) throw error("As expected!");
}


/* Try to assign to a target register. */
node *ASMbuilder::try_assign(Register *__def,value_type __use) {
    if (__use.type == value_type::POINTER) {
        if (__use.pointer.offset == 0) {
            if (__use.pointer.reg == __def) return nullptr;
            else return new move_register {
                __use.pointer.reg,__def
            };
        } else return new arith_immediat {
            arith_base::ADD, __use.pointer.reg,
            __use.pointer.offset, __def
        };
    } else if (__use.type == value_type::GLOBAL) {
        return new load_symbol {
            load_symbol::FULL, __def, __use.global.var
        };
    } else throw error("Fuck!");
}



}