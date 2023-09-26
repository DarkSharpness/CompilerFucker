#include "inline.h"
#include "mem2reg.h"

#include <cstring>

namespace dark::OPT {

/**
 * @brief Copy a definition to another.
 */
IR::definition *inline_visitor::copy(IR::definition *__def) {
    static size_t __n = 0;
    if (!__def) return nullptr;
    if (dynamic_cast <IR::literal *> (__def)
    ||  dynamic_cast <IR::undefined *> (__def)
    ||  dynamic_cast <IR::global_variable *> (__def)) return __def;
    auto [__iter,__result] = val_map.try_emplace(__def);
    if (!__result) return __iter->second;

    /* inl-number */
    const size_t __len = std::__detail::__to_chars_len(++__n);
    std::string  __str(__len + 6, '\0');
    std::strcpy(&__str[0], "%.inl.");
    std::__detail::__to_chars_10_impl(&__str[6], __len, __n);

    if (auto *__var = dynamic_cast <IR::local_variable *> (__def)) {
        auto *__new_var = new IR::local_variable;
        __new_var->name = std::move(__str);
        __new_var->type = __var->type;
        __iter->second  = __new_var;
        return __new_var;
    } else if (auto *__tmp = dynamic_cast <IR::temporary *> (__def)) {
        auto *__new_tmp = new IR::temporary;
        __new_tmp->name = std::move(__str);
        __new_tmp->type = __tmp->type;
        __iter->second  = __new_tmp;
        return __new_tmp;
    }

    throw error("This should never happen!");
}

IR::block_stmt *inline_visitor::create_block(IR::block_stmt *__old) {
    static size_t __n = 0;
    auto &__ref    = blk_map[__old];
    if (__ref) return __ref;

    std::string_view __suffix = __old ? __old->label : "";
    if(__suffix.substr(0,7) == "inline-") __suffix = __suffix.substr(7);

    auto *__block  = new IR::block_stmt;
    __block->label = string_join(
        "inline-",
        std::to_string(__n++),
        __suffix
    );

    auto *__SSA    = get_impl_ptr <SSAbuilder> ();
    auto *__node   = __SSA->create_node(__block);
    __block->set_impl_ptr(__node);
    return __ref = __block;
}



inline_visitor::inline_visitor
    (IR::call_stmt *__call,IR::function *__func,void *__this) {
    set_impl_ptr (__this);
    dest = __call->dest;
    size_t __n = 0;
    for(auto __args : __func->args) val_map[__args] = __call->args[__n++];
    visitFunction(__func);
}

void inline_visitor::visitBlock(IR::block_stmt *__block) {
    top = create_block(__block);
    for(auto __stmt : __block->stmt) visit(__stmt);
}

void inline_visitor::visitFunction(IR::function *__func) {
    std::vector <IR::block_stmt *> __expr;

    if (dest) {
        phi       = new IR::phi_stmt;
        phi->dest = dest;
    } else phi    = nullptr;

    info.exit  = create_block(nullptr);

    for(auto __block : __func->stmt) {
        visitBlock(__block);
        __expr.push_back(top);
    }

    info.entry = create_block(__func->stmt[0]);

    if (phi) info.exit->emplace_new(phi);
    info.exit->emplace_new(new IR::return_stmt);
    __expr.push_back(info.exit);

    /* Remove the useless return. */
    info.exit->stmt.pop_back();
    info.list = std::move(__expr);
}


void inline_visitor::visitInit(IR::initialization *) {}

void inline_visitor::visitCompare(IR::compare_stmt *__stmt) {
    auto *__new_stmt = new IR::compare_stmt;
    __new_stmt->op   = __stmt->op;
    __new_stmt->dest = safe_cast <IR::temporary *> (copy(__stmt->dest));
    __new_stmt->lvar  = copy(__stmt->lvar);
    __new_stmt->rvar  = copy(__stmt->rvar);
    top->stmt.push_back(__new_stmt);
}

void inline_visitor::visitBinary(IR::binary_stmt *__stmt) {
    auto *__new_stmt = new IR::binary_stmt;
    __new_stmt->op   = __stmt->op;
    __new_stmt->dest = safe_cast <IR::temporary *> (copy(__stmt->dest));
    __new_stmt->lvar  = copy(__stmt->lvar);
    __new_stmt->rvar  = copy(__stmt->rvar);
    top->stmt.push_back(__new_stmt);
}

void inline_visitor::visitJump(IR::jump_stmt *__stmt) {
    auto *__new_stmt = new IR::jump_stmt;
    __new_stmt->dest = create_block(__stmt->dest);
    top->stmt.push_back(__new_stmt);
}

void inline_visitor::visitBranch(IR::branch_stmt *__stmt) {
    auto *__new_stmt = new IR::branch_stmt;
    __new_stmt->cond = copy(__stmt->cond);
    __new_stmt->br[0] = create_block(__stmt->br[0]);
    __new_stmt->br[1] = create_block(__stmt->br[1]);
    top->stmt.push_back(__new_stmt);
}

void inline_visitor::visitCall(IR::call_stmt *__stmt) {
    auto *__new_stmt = new IR::call_stmt;
    __new_stmt->dest = dynamic_cast <IR::temporary *> (copy(__stmt->dest));
    __new_stmt->func = __stmt->func;
    for(auto __arg : __stmt->args) __new_stmt->args.push_back(copy(__arg));
    top->stmt.push_back(__new_stmt);
}

void inline_visitor::visitLoad(IR::load_stmt *__stmt) {
    auto *__new_stmt = new IR::load_stmt;
    __new_stmt->dst = safe_cast <IR::temporary *> (copy(__stmt->dst));
    __new_stmt->src = dynamic_cast <IR::non_literal *> (copy(__stmt->src));
    top->stmt.push_back(__new_stmt);
}

void inline_visitor::visitStore(IR::store_stmt *__stmt) {
    auto *__new_stmt = new IR::store_stmt;
    __new_stmt->dst = dynamic_cast <IR::non_literal *> (copy(__stmt->dst));
    __new_stmt->src = copy(__stmt->src);
    top->stmt.push_back(__new_stmt);
}

void inline_visitor::visitReturn(IR::return_stmt *__stmt) {
    auto *__new_stmt = new IR::jump_stmt;
    __new_stmt->dest = info.exit;    
    top->stmt.push_back(__new_stmt);
    if (!__stmt->rval) return; /* Void type. */
    phi->cond.push_back({copy(__stmt->rval),top});
}

void inline_visitor::visitAlloc(IR::allocate_stmt *__stmt) {
    auto *__new_stmt = new IR::allocate_stmt;
    __new_stmt->dest = safe_cast <IR::local_variable *> (copy(__stmt->dest));
    top->stmt.push_back(__new_stmt);
}

void inline_visitor::visitGet(IR::get_stmt *__stmt) {
    auto *__new_stmt = new IR::get_stmt;
    __new_stmt->dst  = safe_cast <IR::temporary *> (copy(__stmt->dst));
    __new_stmt->src  = copy(__stmt->src);
    __new_stmt->idx  = copy(__stmt->idx);
    __new_stmt->mem  = __stmt->mem;
    top->stmt.push_back(__new_stmt);
}

void inline_visitor::visitPhi(IR::phi_stmt *__stmt) {
    auto *__phi_stmt = new IR::phi_stmt;
    __phi_stmt->dest = safe_cast <IR::temporary *> (copy(__stmt->dest));
    for(auto [__value,__block] : __stmt->cond)
        __phi_stmt->cond.push_back({copy(__value),create_block(__block)});
    top->stmt.push_back(__phi_stmt);
}

void inline_visitor::visitUnreachable(IR::unreachable_stmt *) {
    top->stmt.push_back(IR::create_unreachable());
}

}