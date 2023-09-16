#include "peephole.h"


namespace dark::OPT {


/**
 * @brief Only those with the same type may share address.
 * What's more , global variables will not share the address.
 * Local variables will not share the address with global variables.
 * Those generated from call are dangerous! It may share the
 * address with any other variables.
 * 
 */
bool local_optimizer::may_share_address
    (IR::non_literal *__lhs,IR::non_literal *__rhs) {
    if (!(__lhs->type == __rhs->type)) return false;
    else return true;
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
    auto __code = __call->func->is_side_effective();
    if (__code < 4) return;
    if (__code > 4) runtime_assert("Not implemented yet!");

    /* An extreme version: Assume that all mem_info are dead. */
    mem_info.clear();
}

void local_optimizer::visitLoad(IR::load_stmt *__load) {
    auto &__vec = mem_info[__load->src];
    if (!__vec.last || __vec.corrupted)
        return static_cast <void> (__vec = {__load,false});

    remove_set.insert(__load);
    /* Load after store. */
    if (auto __store = dynamic_cast <IR::store_stmt *> (__vec.last)) {
        for(auto __use : use_map[__load->dst])
            __use->update(__load->dst,__store->src);
        __vec.last = __load;
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
        /**
         * If not identical, check the type!
         * Only if the type is identical,
         * and the memory may be overlapping,
         * the memory operation will be affected.
         * 
        */
        if (may_share_address(__var,__store->dst))
            __vec.corrupted = true;
    }

    auto &__vec = mem_info[__store->dst];
    if (!__vec.last) return static_cast <void> (__vec = {__store,false});

    /* Store after load. */
    if (auto __load = dynamic_cast <IR::load_stmt *> (__vec.last)) {
        /* Useless store: Eliminate it if not corrupted. */
        if (__load->dst == __store->src && !__vec.corrupted)
            return static_cast <void> (remove_set.insert(__store));
    } else { /* Store after store. */
        auto __prev = safe_cast <IR::store_stmt *> (__vec.last);
        /* Useless previous store: Eliminate it! */
        remove_set.insert(__prev);
    }

    __vec = {__store,false};
}

void local_optimizer::visitReturn(IR::return_stmt *) {}

void local_optimizer::visitAlloc(IR::allocate_stmt *) {}

void local_optimizer::visitGet(IR::get_stmt *__stmt) {
    if (replace_common(__stmt)) try_remove(__stmt);
}

void local_optimizer::visitPhi(IR::phi_stmt *) {}

void local_optimizer::visitUnreachable(IR::unreachable_stmt *) {}

}