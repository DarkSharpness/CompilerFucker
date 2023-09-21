#include "sccp.h"


namespace dark::OPT {


/**
 * @brief Sparse conditional constant propagation.
 * It will not change CFG and only do constant propagation.
 * Nevertheless, it still take use of constant branch to
 * better spread constant.
 * 
 */
constant_propagatior::constant_propagatior
    (IR::function *__func,node *__entry) {
    if (__func->is_unreachable()) return;

    collect_use(__func);
    init_info(__func);

    while(!CFG_worklist.empty()) {
        do update_CFG(); while(!CFG_worklist.empty());
        while(!SSA_worklist.empty()) update_SSA();
    }

    update_constant(__func);
}


/**
 * @brief Returns if constant_calculator can work on this node
 * and return a non-nullptr (aka. constant/undefined) value.
 * Currently, only binary/compare/phi statements are supported.
 * 
 * @param __node The IR node to be checked.
 * @return As described above.
 */
bool constant_propagatior::may_go_constant(IR::node *__node) {
    if (dynamic_cast <IR::binary_stmt  *> (__node)) return true;
    if (dynamic_cast <IR::compare_stmt *> (__node)) return true;
    if (dynamic_cast <IR::phi_stmt *>     (__node)) return true;
    if (dynamic_cast <IR::call_stmt *>    (__node)) return true;
    return false;
}


/**
 * @brief Take out an edge from CFG worklist and work on it.
 */
void constant_propagatior::update_CFG() {
    auto [__from,__block] = CFG_worklist.front(); CFG_worklist.pop();
    auto &__visitor = block_map[__block];
    if (__visitor.visit_from(__from)) return;

    auto __beg = __block->stmt.begin();
    auto __end = --__block->stmt.end();

    while(auto __phi = dynamic_cast <IR::phi_stmt *> (*__beg))
        update_PHI(__phi) , ++__beg;

    if (__visitor.visit_count() == 1)
        while(__beg != __end) visit_node(*__beg++);

    if (auto __jump = dynamic_cast <IR::jump_stmt *> (*__beg))
        CFG_worklist.push({__block,__jump->dest});

    else if (auto __br = dynamic_cast <IR::branch_stmt *> (*__beg))
        visit_branch(__br);
}


/**
 * @brief Take out a node from SSA worklist and work on it.
 */
void constant_propagatior::update_SSA() {
    auto __node = SSA_worklist.front(); SSA_worklist.pop();
    if (auto __phi = dynamic_cast <IR::phi_stmt *> (__node))
        update_PHI(__phi);
    else if (node_map[__node]->visit_count())
        visit_node(__node);
}


/**
 * @brief Visit a phi node.
 */
void constant_propagatior::update_PHI(IR::phi_stmt *__phi) {
    auto *__info = node_map[__phi];
    if (!__info->visit_count()) return;

    __cache.clear();
    for(auto [__value,__block] : __phi->cond)
        if(__info->find(__block))
            __cache.push_back(get_value(__value));

    try_update_value(__phi);
}


void constant_propagatior::visit_node(IR::node *__node) {
    /* Only those that may go constant will be updated. */
    if (auto __br = dynamic_cast <IR::branch_stmt *> (__node))
        return visit_branch(__br);

    if (!may_go_constant(__node)) return;

    __cache.clear();
    for(auto __use : __node->get_use())
        __cache.push_back(get_value(__use));

    try_update_value(__node);
}


void constant_propagatior::try_update_value(IR::node *__node) {
    auto __new = calc.work(__node,__cache);
    auto __def = __node->get_def();

    /* Self definition means undefined! Nothing to do! */
    if (__new == __def) return;
    auto &__info = use_map.at(__def);

    /* Merge the definition according to given rule. */
    __new = merge_definition(__info.new_def,__new);
    if (__info.new_def == __new) return;

    /* Update case! */
    __info.new_def = __new;
    for(auto __use : __info.use_list) SSA_worklist.push(__use);
}


IR::definition *constant_propagatior::get_value(IR::definition *__def) {
    auto __tmp = dynamic_cast <IR::temporary *> (__def);

    if (!__tmp) /* Only temporaries can be remapped. */
        return dynamic_cast <IR::undefined *> (__def)
            || dynamic_cast <IR::literal *>   (__def)
            || dynamic_cast <IR::variable *>  (__def) ? __def : nullptr;

    /* Find the value in the temporary map. */
    auto __new = use_map[__tmp].new_def;

    /* In worst case, a temporary can still be itself. */
    return __new ? __new : __def;
}


void constant_propagatior::collect_use(IR::function *__func) {
    for(auto __block : __func->stmt)
        for(auto __stmt : __block->stmt) {
            auto __def = __stmt->get_def();
            if (__def) use_map[__def].def_node = __stmt;
            for(auto __use : __stmt->get_use())
                if(auto __tmp = dynamic_cast <IR::temporary *> (__use))
                    use_map[__tmp].use_list.push_back(__stmt);
        }
}


void constant_propagatior::init_info(IR::function *__func) {
    /* Only those undefined or possibly go constant
        will be assigned undefined. Others are null (non-constant). */
    for(auto &&[__def,__info] : use_map)
        if (!__info.def_node || may_go_constant(__info.def_node))
            __info.new_def = IR::create_undefined(__def->type);

    for(auto __block : __func->stmt) {
        auto &__info = block_map[__block];
        for(auto __node : __block->stmt)
            node_map[__node] = &__info;
    }

    /* Insert to CFG */
    CFG_worklist.push({nullptr,__func->stmt.front()});
}


void constant_propagatior::update_constant(IR::function *__func) {
    std::vector <IR::node *> __vec;
    for(auto __block : __func->stmt) {
        for(auto __node : __block->stmt) {
            for(auto __use : __node->get_use()) {
                auto *__val = get_value(__use);
                if (auto __lit = dynamic_cast <IR::definition *> (__val);
                    __lit != nullptr && __use != __lit)
                    __node->update(__use,__lit);
            }
        }
    }
}


void constant_propagatior::visit_branch(IR::branch_stmt *__br) {
    auto __var = get_value(__br->cond);

    /* No branch to take now. */
    if (dynamic_cast <IR::undefined *> (__var)) return;

    auto *__block = get_block(__br);
    /* Take only one branch. */
    if (auto __bool = dynamic_cast <IR::boolean_constant *> (__var))
        CFG_worklist.push({__block,__br->br[!__bool->value]});
    else { /* Take both is non-constant. */
        CFG_worklist.push({__block,__br->br[0]});
        CFG_worklist.push({__block,__br->br[1]});
    }
}


}