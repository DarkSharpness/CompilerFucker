#include "peephole.h"


namespace dark::OPT {


local_optimizer::local_optimizer(IR::function *__func,node *) {
    for (auto __block : __func->stmt) {
        for(auto __stmt : __block->stmt) {
            auto __vec = __stmt->get_use();
            for(auto __use : __vec)
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

    for(auto __stmt : __block->stmt) {
        visit(__stmt);
        auto *__def = __stmt->get_def();
        if (__def == nullptr) continue;
        auto *__tmp = get_def_value(__def);
        if (__def != __tmp)
            for(auto __use : use_map[__def])
                __use->update(__def,__tmp);                
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


void local_optimizer::visitCompare(IR::compare_stmt *__stmt) {

}

void local_optimizer::visitBinary(IR::binary_stmt *__stmt) {
    if (!update_bin(__stmt)) replace_common(__stmt);
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

}

void local_optimizer::visitStore(IR::store_stmt *) {}

void local_optimizer::visitReturn(IR::return_stmt *) {}

void local_optimizer::visitAlloc(IR::allocate_stmt *) {

}

void local_optimizer::visitGet(IR::get_stmt *__stmt) {
    replace_common(__stmt);
}

void local_optimizer::visitPhi(IR::phi_stmt *) {

}


void local_optimizer::visitUnreachable(IR::unreachable_stmt *) {}


}