#include "peephole.h"
#include "IRinfo.h"

namespace dark::OPT {


/**
 * @brief Only those with the same type may share address.
 * What's more , global variables will not share the address.
 * Local variables will not share the address with global variables.
 * Those generated from call are dangerous! It may share the
 * address with any other variables.
 * 
 * @return 0 if not identical
 *      || 1 if identical
 *      || 2 if possibly identical
 */
size_t local_optimizer::may_share_address
    (IR::non_literal *__lhs,IR::non_literal *__rhs) {
    if (__lhs == __rhs) return 1;
    if (__lhs->type != __rhs->type) return 0;
    auto __result =
        get_min_max(get_address_info(__lhs),get_address_info(__rhs));
    switch(static_cast <unsigned> (__result.min)) {
        case 0: return 0; /* global     */
        case 1: return 2; /* phi case   */
        case 2: return 0; /* allocated  */
        case 3: return 2; /* loaded out */
        case 4: /* From get elementptr. */
            if (__result.max != __result.min) return 0;
            else break;
        case 5: return 2; /* result from function call */
        case 6: /* From function arguments. */
            if (__result.max == __result.min) return 2;
        case 7: return 0; /* Malloc special case. */
        default: throw error("Unexpected");
    }

    /* Both from get-elementptr. */
    auto *__get_lhs = safe_cast <IR::get_stmt *>
        (use_map.at(safe_cast <IR::temporary *> (__lhs)).def_node);
    auto *__get_rhs = safe_cast <IR::get_stmt *>
        (use_map.at(safe_cast <IR::temporary *> (__rhs)).def_node);

    /* The same member access. */
    if (__get_lhs->mem != __get_rhs->mem) return 0;

    /* From same source type. */
    if (__get_lhs->src->get_value_type()
    !=  __get_rhs->src->get_value_type()) return 0;

    if (__get_lhs->idx != __get_rhs->idx) {
        if (dynamic_cast <IR::literal *> (__get_lhs->idx)
        &&  dynamic_cast <IR::literal *> (__get_rhs->idx))
            return 0;
        /* From the possibly identical index. */
        auto __lvar = dynamic_cast
            <IR::non_literal *> (__get_lhs->src);
        auto __rvar = dynamic_cast
            <IR::non_literal *> (__get_rhs->src);
        if (!__lvar || !__rvar) return 0;
        return may_share_address(__lvar,__rvar) ? 2 : 0;
    } else {
        /* From identical index.  */
        auto __lvar = dynamic_cast
            <IR::non_literal *> (__get_lhs->src);
        auto __rvar = dynamic_cast
            <IR::non_literal *> (__get_rhs->src);
        if (!__lvar || !__rvar) return 0;
        return may_share_address(__lvar,__rvar);            
    }
}

local_optimizer::address_type
    local_optimizer::get_address_info(IR::non_literal *__var) {
    auto [__iter,__result] = addr_map.try_emplace(__var);
    if (!__result) return __iter->second;
    /* Build up the address info. */
    
    if (dynamic_cast <IR::function_argument *> (__var))
        return __iter->second = address_type::ARGV;
    if (dynamic_cast <IR::global_variable *> (__var))
        return __iter->second = address_type::GLOBAL;
    if (dynamic_cast <IR::local_variable *> (__var))
        return __iter->second = address_type::LOCAL;

    auto *__def = safe_cast <IR::temporary *> (__var);
    auto *__stmt = use_map.at(__def).def_node;

    if (auto __call = dynamic_cast <IR::call_stmt *> (__stmt)) {
        if (__call->func->is_builtin) {
            auto &&__name = __call->func->name;
            if (__name == "__new_array4__"
            ||  __name == "__new_array1__"
            ||  __name == "malloc")
                return __iter->second = address_type::NEW;
        }

        /*  */
        return __iter->second = address_type::CALL;
    } else if (auto __load = dynamic_cast <IR::load_stmt *> (__stmt)) {
        return __iter->second = address_type::LOAD;
    } else if (auto __phi = dynamic_cast <IR::phi_stmt *> (__stmt)) {
        return __iter->second = address_type::PHI;
    } else {
        auto __get = safe_cast <IR::get_stmt *> (__stmt);
        return __iter->second = address_type::GET;
    }
}


local_optimizer::usage_info *
    local_optimizer::get_use_info(IR::definition *__def) {
    static constexpr size_t SZ = 4; /* Cache size! */
    static usage_info  __cache[SZ];
    static size_t      __count = 0;
    if (__count == SZ) __count = 0;

    if (auto *__tmp = dynamic_cast <IR::temporary *> (__def)) {
        auto __iter = use_info.find(__tmp);
        if (__iter != use_info.end())
            return std::addressof(__cache[__count++] = __iter->second);
    }
    return std::addressof(__cache[__count++] =
        {.new_def = __def,.def_node = nullptr,.neg_flag = false}
    );
}


local_optimizer::local_optimizer(IR::function *__func,node *) {
    if (__func->is_unreachable()) return;

    for (auto __block : __func->stmt) {
        for(auto __stmt : __block->stmt) {
            auto *__def = __stmt->get_def();
            if (__def) use_map[__def].def_node = __stmt;
            for(auto __use : __stmt->get_use())
                if(auto __tmp = dynamic_cast <IR::temporary *> (__use))
                    use_map[__tmp].push_back(__stmt);
        }
    }

    for(auto __block : __func->stmt) visitBlock(__block);

    std::vector <IR::node *> __new_list;
    for(auto __block : __func->stmt) {
        for(auto __stmt : __block->stmt)
            if (!remove_set.count(__stmt))
                __new_list.push_back(__stmt);
        __new_list.swap(__block->stmt);
        __new_list.clear();
    }
}


void local_optimizer::visitBlock(IR::block_stmt *__block) {
    use_info.clear();
    mem_info.clear();
    common_map.clear();
    for(auto __stmt : __block->stmt) visit(__stmt);
}


void local_optimizer::visitCompare(IR::compare_stmt *__stmt) {

}

void local_optimizer::visitBinary(IR::binary_stmt *__stmt) {
    if (!update_bin(__stmt)) replace_common(__stmt);
    try_remove(__stmt);
}

void local_optimizer::visitJump(IR::jump_stmt *) {}

void local_optimizer::visitBranch(IR::branch_stmt *) {}

void local_optimizer::visitCall(IR::call_stmt *__call) {
    if (__call->func->is_builtin) return;
    auto __ptr = __call->func->get_impl_ptr <function_info> ();
    /* An extreme version: Assume that all mem_info are dead. */
    if (!__ptr || !__ptr->real_info) { mem_info.clear(); return; }
    auto *__info = __ptr->real_info;

    /* Dangerous! Load/Store on the same type! */
    for(auto &[__var,__ref] : mem_info) {
        __ref.set_load();
        /* May operate on the same address. */
        if (__info->store_name.count(__var->type.name()))
            __ref.reset(nullptr);
    }

    for(auto [__global,__state] : __info->used_global_var) {
        auto &__ref = mem_info[__global];
        __ref.set_load();
        if (__state & __info->STORE) __ref.reset(nullptr);
    }
}

void local_optimizer::visitLoad(IR::load_stmt *__load) {
    if (__load->is_undefined_behavior()) return;
    for(auto &&[__var,__vec] : mem_info) {
        if (__var == __load->src) continue;
        if (!dynamic_cast <IR::store_stmt *> (__vec.last)) continue;
        if (__vec.corrupt_load()) continue;

        /* This will only affect store after store */
        if (may_share_address(__var,__load->src)) __vec.set_load();
    }

    auto &__vec = mem_info[__load->src];

    /* If corrupt store, nothing shall be done. */
    if (!__vec.last || __vec.corrupt_store()) return __vec.reset(__load);

    __vec.corrupt_reset();
    remove_set.insert(__load);

    /* Load after store. */
    if (auto __store = dynamic_cast <IR::store_stmt *> (__vec.last)) {
        for(auto __use : use_map[__load->dst])
            __use->update(__load->dst,__store->src);
    } else { /* Load after load. */
        auto __prev = safe_cast <IR::load_stmt *> (__vec.last);
        for(auto __use : use_map[__load->dst])
            __use->update(__load->dst,__prev->dst);
    }
}

void local_optimizer::visitStore(IR::store_stmt *__store) {
    if (__store->is_undefined_behavior()) return;
    /* First of all, all unsafe memory operation will be affected. */
    for(auto &&[__var,__vec] : mem_info) {
        if (__var == __store->dst) continue;
        if (__vec.corrupt_store()) continue;

        /* This will affect everything except store after after. */
        if (may_share_address(__var,__store->dst)) __vec.set_store();
    }

    auto &__vec = mem_info[__store->dst];
    if (!__vec.last) return __vec.reset(__store);

    /* Store after load. */
    if (auto __load = dynamic_cast <IR::load_stmt *> (__vec.last)) {
        /* Special case load and store are identical. */
        if (__load->dst == __store->src && !__vec.corrupt_store()) {
            remove_set.insert(__store);
            return __vec.corrupt_reset();
        }
    } else { /* Store after store. */
        auto __prev = safe_cast <IR::store_stmt *> (__vec.last);
        /* Useless previous store: Eliminate it! */
        if (!__vec.corrupt_load()) remove_set.insert(__prev);
    }

    return __vec.reset(__store);
}

void local_optimizer::visitReturn(IR::return_stmt *) {}

void local_optimizer::visitAlloc(IR::allocate_stmt *) {}

void local_optimizer::visitGet(IR::get_stmt *__stmt) {
    if (replace_common(__stmt)) try_remove(__stmt);
}

void local_optimizer::visitPhi(IR::phi_stmt *) {}

void local_optimizer::visitUnreachable(IR::unreachable_stmt *) {}

}