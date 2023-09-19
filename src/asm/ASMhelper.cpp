#include "ASMbuilder.h"


namespace dark::ASM {


pointer_address ASMbuilder::get_pointer_address(IR::non_literal *__def) {
    if (!__def) return pointer_address { get_physical(0), 0 };
    else if (auto __local = dynamic_cast <IR::local_variable *> (__def)) {
        return pointer_address { get_physical(2), offset_map.at(__local) };
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


virtual_register *ASMbuilder::get_virtual() {
    return top_asm->create_virtual();
}


virtual_register *ASMbuilder::get_virtual(IR::non_literal *__var) {
    if (auto __temp = dynamic_cast <IR::temporary *> (__var))
        if (getelement_map.count(__temp));
            runtime_assert("Get results shouldn't be passed out!");

    auto &__ref = temp_map[__var];
    return __ref ? __ref : (__ref = get_virtual());
}


virtual_register *ASMbuilder::get_virtual(IR::literal *__lit) {
    auto *__ans = get_virtual();
    if (auto __ptr = dynamic_cast <IR::pointer_constant *> (__lit);
        __ptr != nullptr && __ptr->var != nullptr) {
        top_block->emplace_back(new load_symbol {
            load_symbol::FULL, __ans,
            const_cast <IR::global_variable *> (__ptr->var)
        });
        return __ans;
    } else {
        top_block->emplace_back(new load_immediate {
            __ans, __lit->get_constant_value()
        });
        return __ans;
    }
}


block *ASMbuilder::get_edge(IR::block_stmt *prev,IR::block_stmt *next) {
    if (!dynamic_cast <IR::phi_stmt *> (next->stmt.front()))
        return get_block(next);
    auto &__ref = branch_map[prev];

    if (__ref.key[0] == next) return __ref.val[0];
    if (__ref.key[1] == next) return __ref.val[1];

    block *__temp = nullptr;
    if (__ref.key[0] == nullptr) {
        __ref.key[0] = next;
        __ref.val[0] = __temp = get_virtual_block <1> (prev);
    } else if (__ref.key[1] == nullptr) {
        __ref.key[1] = next;
        __ref.val[1] = __temp = get_virtual_block <2> (prev);
    } else runtime_assert("WTF ???");

    __temp->phi.op = dummy_expression::USE;
    /* Only expression: jump to the target block after usage. */
    __temp->expression = { new jump_expression {get_block(next)} };
    return __temp;
}


/**
 * @brief Return the virtual register of a definition.
 * @param __def Input definition.
 * @return The virtual register of the definition.
 * If __def = nullptr, return a new virtual register.
 * If __def = immediate, insert a load_immedate and return the virtual register.
 * If __def = non_literal, return the virtual register binded to it.
 * Else throw an exception.
 */
virtual_register *ASMbuilder::get_virtual(IR::definition *__def) {
    if (dynamic_cast <IR::global_variable *> (__def)
    ||  dynamic_cast <IR::local_variable  *> (__def))
        runtime_assert("???");
    if (!__def) return get_virtual();
    else if (auto __tmp = dynamic_cast <IR::non_literal *> (__def))
        return get_virtual(__tmp);
    else
        return get_virtual(safe_cast <IR::literal *> (__def));
}




}