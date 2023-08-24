#include "mem2reg.h"

#include <algorithm>
#include <iterator>
#include <queue>

namespace dark::MEM {


dominate_maker::dominate_maker(node *__entry) {
    dfs(__entry);
    std::reverse(node_rpo.begin(), node_rpo.end());
    runtime_assert("Entry node is not the first node in rpo!",
        node_rpo.front() == __entry);

    /* Work out the dominant set iteratively. */
    iterate(__entry);

    for(auto __node : node_rpo)
        for(auto __prev : __node->prev)
            for(auto __temp : __prev->dom)
                if(!__node->dom.count(__temp))
                    __temp->fro.insert(__node);
    // debug_print(std::cerr);

    /* Collect the defs first and spread the defs. */
    for(auto __node : node_rpo) {
        auto __iter = node_def.try_emplace(__node,
            info_collector::collect_def(__node->block)).first;
        spread_def(__node,__iter->second);
    }

    /* Spread the extra definition made by phi. */
    spread_phi();
}

void dominate_maker::spread_def(node *__node,std::set <IR::variable *> &__set) {
    for(auto __next : __node->fro) {
        auto &__map = node_phi[__next];
        for(auto *__var : __set) {
            auto &__phi = __map[__var];
            if(__phi == nullptr)
                __phi = new IR::phi_stmt;
        }
    }
}


void dominate_maker::spread_phi() {
    std::queue <node *> __q;
    for(auto __p : node_rpo) __q.push(__p);
    while(!__q.empty()) {
        auto __node = __q.front(); __q.pop();
        auto &__tmp = node_phi[__node];

        /* Visit all dominant frontier. */
        for(auto __next : __node->fro) {
            bool __flag = false;
            auto &__map = node_phi[__next];

            /* Update by the newly defined phi(variable) */
            for(auto [__var,__] : __tmp) {
                auto &__phi = __map[__var];
                if(__phi == nullptr) {
                    __phi = new IR::phi_stmt;
                    __flag = true;
                }
            }

            /* This node will be updated. */
            if(__flag) __q.push(__next);
        }
    }
}

/* Boring iteration~ */
void dominate_maker::iterate(node *__entry) {
    for(auto __node : node_rpo)
        if(__node != __entry)
            __node->dom = node_set;
    __entry->dom = {__entry};
    do {
        update_tag = false;
        for(auto __node : node_rpo) {
            if(__node == __entry) continue;
            else update(__node);
        }
    } while(update_tag);
}


void dominate_maker::dfs(node *__node) {
    /* Already in the set. */
    if(node_set.insert(__node).second == false) return;
    /* Push the node into the set. */
    for(auto __p : __node->next) dfs(__p);
    /* Post order. */
    node_rpo.push_back(__node);
}


void dominate_maker::update(node *__node) {
    runtime_assert("Invalid node!",__node->prev.size() > 0);
    std::set <node *> __dom = __node->prev[0]->dom;
    for(size_t i = 1 ; i < __node->prev.size() ; ++i) {
        auto &__set = __node->prev[i]->dom;
        std::set <node *> __tmp;
        std::set_intersection (
            __dom.begin(), __dom.end(),
            __set.begin(), __set.end(),
            std::insert_iterator(__tmp, __tmp.begin())
        );
        __dom = std::move(__tmp);
    }
    __dom.insert(__node);
    if(__dom != __node->dom) {
        __node->dom = std::move(__dom);
        update_tag  = true;
    }
}


void graph_builder::visitFunction(IR::function *ctx) {
    for(auto __block : ctx->stmt) visitBlock(__block);
}

void graph_builder::visitBlock(IR::block_stmt *ctx) {
    top = create_node(ctx);
    for(auto __stmt : ctx->stmt) visit(__stmt);
}

void graph_builder::visitInit(IR::initialization *ctx) {}
void graph_builder::visitCompare(IR::compare_stmt *ctx) {}
void graph_builder::visitBinary(IR::binary_stmt *ctx) {}

void graph_builder::visitJump(IR::jump_stmt *ctx) {
    link(top, create_node(ctx->dest));
}

void graph_builder::visitBranch(IR::branch_stmt *ctx) {
    link(top, create_node(ctx->br[0]));
    link(top, create_node(ctx->br[1]));
}

void graph_builder::visitCall(IR::call_stmt *ctx) {}

void graph_builder::visitLoad(IR::load_stmt *ctx) {}

void graph_builder::visitStore(IR::store_stmt *ctx) {

}

void graph_builder::visitReturn(IR::return_stmt *ctx) {

}

void graph_builder::visitAlloc(IR::allocate_stmt *ctx) {

}

void graph_builder::visitGet(IR::get_stmt *ctx) {

}

void graph_builder::visitPhi(IR::phi_stmt *ctx) {

}

void graph_builder::visitUnreachable(IR::unreachable_stmt *ctx) {

}



}