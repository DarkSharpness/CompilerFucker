#include "sccp.h"


namespace dark::OPT {

/**
 * @brief Returns if constant_calculator can work on this node
 * and return a non-nullptr (aka. constant/undefined) value.
 * Currently, only binary/compare/phi statements are supported.
 * 
 * @param __node The IR node to be checked.
 * @return As described above.
 */
inline bool constant_propagatior::may_go_constant(IR::node *__node) {
    if (dynamic_cast <IR::binary_stmt  *> (__node)) return true;
    if (dynamic_cast <IR::compare_stmt *> (__node)) return true;
    if (dynamic_cast <IR::phi_stmt *>     (__node)) return true;
    return false;
}


constant_propagatior::constant_propagatior
    (IR::function *__func,node *) {
    collect_use(__func);
    init_info(__func);

    /* Start working. */
    __cache.reserve(2);
    while (!block_list.empty()) update_block();

    /* Now, all blocks are updated. */
    update_constant(__func);
}


IR::definition *get_phi_value(IR::phi_stmt *__phi,IR::block_stmt *__pre) {
    for(auto [__val,__src] : __phi->cond)
        if (__src == __pre) return __val;
    runtime_assert("This should never never happen!");
    return nullptr;
}


void constant_propagatior::update_block() {
    auto [__block,__prev,__info] = block_list.front(); block_list.pop();
    auto __beg = __block->stmt.begin();
    auto __end = --__block->stmt.end();


    { /* Just update all phi statements parallelly. */
        /* If phi statement, update branch of that side. */
        std::vector <std::pair <IR::temporary *,IR::definition *>> __remap;
        while(auto __phi = dynamic_cast <IR::phi_stmt *> (*__beg)) {
            auto __val = get_phi_value(__phi,__prev);
            __remap.push_back({__phi->dest,__info->get_literal(__val)});
            ++__beg;
        }
        for(auto __pair : __remap)
            if(__info->update(__pair.first,__pair.second))
                spread_state(__info,__pair.first);
    }


    /* Update all statements except the exit. */
    while(__beg != __end) try_spread(__info,*__beg++);
    /* Special case! If merge failed, no more visiting. */
    if (!merge_info(__block,__info)) return delete __info;


    /* Visit the last node (CFG related node.) */
    auto __last = *__beg;
    if (auto __jump = dynamic_cast <IR::jump_stmt *> (__last))
        block_list.push({__jump->dest,__block,__info});

    else if (auto __br = dynamic_cast <IR::branch_stmt *> (__last)) {
        auto __cond = __info->get_literal(__br->cond);

        /* WTF? Undefined branch ?? Just cut it all! */
        if(dynamic_cast <IR::undefined *> (__cond)) {
            warning("I never expected this to happen! (Undefined branch)");
            return delete __info;
        }

        /* Normal case now. */
        if (auto __bool = dynamic_cast <IR::boolean_constant *> (__cond))
            block_list.push({__br->br[!__bool->value],__block,__info});
        else { /* Both branches are reachable! */
            block_list.push({__br->br[0],__block,new auto(*__info)});
            block_list.push({__br->br[1],__block,__info});
        }
    } else { /* In theory, there is no more unreachable now...... */
        safe_cast <IR::return_stmt *> (__last);
        return delete __info;
    }
}


void constant_propagatior::spread_state
    (constant_info *__info,IR::temporary *__def) {
    /* Non-constant / Constant. */
    IR::literal *__val = dynamic_cast <IR::literal *>
        (__info->value_map.at(__def));

    /* Go to all usage and tries to update. */
    for(auto __use : use_map[__def].use_list)
        try_spread(__info,__use);
}


void constant_propagatior::try_spread
    (constant_info *__info,IR::node *__stmt) {
    /* A new definition of the statement. */
    if (!may_go_constant(__stmt)) return;
    /* This is a temporary optimization!!! */
    if (dynamic_cast <IR::phi_stmt *> (__stmt)) return;

    auto __def = __stmt->get_def();

    __cache.clear();
    for(auto __tmp : __stmt->get_use())
        __cache.push_back(__info->get_literal(__tmp));

    if(__info->update(__def,calc.work(__stmt,__cache))) 
        spread_state(__info,__def);
}


bool constant_propagatior::merge_info
    (IR::block_stmt *__block,constant_info *__info) {
    auto [__iter,__succ] = info_map.try_emplace(__block,*__info);
    if (__succ) return true;
    /* Now __succ is naturally false. */

    auto __beg = __info->value_map.begin();
    for(auto &&[__key,__old] : __iter->second.value_map) {
        runtime_assert("I misunderstood STL......",__beg->first == __key);
        auto &__new = (__beg++)->second;
        /* Nothing to update if identical or old is null. */
        if (__new == __old || __old == nullptr) continue;

        __succ = true;
        /* Undefined case. */
        if (dynamic_cast <IR::undefined *> (__old)) {
            __old = __new;
        } else if (dynamic_cast <IR::undefined *> (__new)) {
            __new = __old;
        } else {
            __old = __new = nullptr;
        }
    } return __succ;
}


void constant_propagatior::collect_use(IR::function *__func) {
    for(auto __block : __func->stmt)
        for(auto __stmt : __block->stmt) {
            auto __def = __stmt->get_def();
            if(__def) use_map[__def].def_node = __stmt;
            for(auto __use : __stmt->get_use())
                if(auto __tmp = dynamic_cast <IR::temporary *> (__use))
                    use_map[__tmp].use_list.push_back(__stmt);
        }
}


void constant_propagatior::init_info(IR::function *__func) {
    info_map.reserve(__func->stmt.size());
    /* Set all as undefined! */
    auto *__info = new constant_info;

    /* Only when may go constant-able will be assigned undefined. */
    for(auto &&__pair : use_map)
        __info->value_map[__pair.first] =
            may_go_constant(__pair.second.def_node) ?
                IR::create_undefined(__pair.first->type) : nullptr;

    auto *__entry = __func->stmt.front();
    block_list.push({__entry,nullptr,__info});
}


void constant_propagatior::update_constant(IR::function *__func) {
    std::vector <IR::node *> __vec;
    for(auto __block : __func->stmt) {
        auto  __iter = info_map.find(__block);
        if (__iter == info_map.end()) continue;
        auto &__info = __iter->second;
        for(auto __stmt : __block->stmt) {
            /* Useless expression is eliminated. */
            auto *__def = __stmt->get_def();
            if (__def && __info.value_map.at(__def)) continue;

            /* Otherwise should keep down. */
            __vec.push_back(__stmt);
            auto __uses = __stmt->get_use();
            for(auto *__use : __uses) {
                auto *__val = __info.get_literal(__use);
                if (__val) __stmt->update(__use,__val);
            }
        }
        __vec.swap(__block->stmt);
        __vec.clear();
    }
}


}