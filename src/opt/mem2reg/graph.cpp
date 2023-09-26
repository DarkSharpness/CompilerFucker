#include "mem2reg.h"
#include "dominate.h"

#include <queue>
#include <algorithm>
#include <iterator>

/* Dominate maker */
namespace dark::OPT {

/**
 * @brief Build up the dominant tree only
 * @param __entry The entry node.
 * @param __flag  Whether this is a reverse graph.
 */
dominate_maker::dominate_maker(node *__entry,bool __flag) {
    dfs(__entry);
    std::reverse(node_rpo.begin(), node_rpo.end());

    /* Work out the dominant set iteratively. */
    iterate(__entry);

    if(__flag) { /* Reverse graph, so remove virtual exit from the node. */
        for(auto __node : node_rpo) {
            auto __iter = std::find (
                __node->dom.rbegin(),__node->dom.rend(),__entry
            );
            __node->dom.erase(__iter.base() - 1);
        }
    }

    for(auto __node : node_rpo)
        for(auto __prev : __node->prev)
            for(auto __temp : __prev->dom)
                if(__node == __temp || !std::binary_search(
                   __node->dom.begin(),__node->dom.end(),__temp)
                )  __temp->fro.push_back(__node);

    for(auto __node : node_rpo) {
        if(__node == __entry) continue;
        std::sort(__node->fro.begin(),__node->fro.end());
        auto __iter = std::unique(__node->fro.begin(),__node->fro.end());
        __node->fro.resize(__iter - __node->fro.begin());
    }
}


/**
 * @brief Build up the dominant tree and perform
 * SSA build up algorithm.
 * @param __entry The entry node.
 */
dominate_maker::dominate_maker(IR::function *,node *__entry)
    : dominate_maker {__entry,false} {
    /* Collect the defs first and spread the defs. */
    for(auto __node : node_rpo) {
        std::set <IR::variable *> __set;
        info_collector::collect_def(__node->block,__set);
        spread_def(__node,__set);
    }

    /* Spread the extra definition made by phi. */
    spread_phi();

    for(auto __node : node_rpo)
        info_collector::collect_use(__node->block,use_map);

    /* Complete renaming. */
    node_set.clear();
    rename(__entry);

    /* A funny wrapper ~ */
    struct my_iterator : decltype(node_phi[nullptr].begin()) { 
        using base = decltype(node_phi[nullptr].begin());
        my_iterator(base __base) : base(__base) {}
        IR::statement *operator *() { return (base::operator * ()).second; }
    };

    for(auto __node : node_rpo) {
        __node->dom.clear(); /* These 2 data are useless now. */
        __node->fro.clear(); /* These 2 data are useless now. */

        auto &__map = node_phi[__node];
        __node->block->stmt.insert (
            __node->block->stmt.begin(),
            my_iterator {__map.begin()},
            my_iterator {__map.end()}
        );
    }
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


void dominate_maker::iterate(node *__entry) {
    std::vector <node *> __vec;
    for(auto __node : node_rpo)
        if(__node != __entry) {
            if(__vec.empty()) {
                __vec.assign(node_set.begin(),node_set.end());
                std::sort(__vec.begin(),__vec.end());
            }
            __node->dom = __vec;
        }
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
    std::vector <node *> __dom = __node->prev[0]->dom;
    std::vector <node *> __tmp;

    /* Set intersection operation for all nodes. */
    for(size_t i = 1 ; i < __node->prev.size() ; ++i) {
        auto &__set = __node->prev[i]->dom;
        std::set_intersection (
            __dom.begin(), __dom.end(),
            __set.begin(), __set.end(),
            std::back_inserter(__tmp)
        );
        std::swap(__tmp,__dom); __tmp.clear();
    }

    /* Add node to current set. */
    auto __iter = std::lower_bound(__dom.begin(),__dom.end(),__node);
    if (__iter == __dom.end() || *__iter != __node)
        __dom.insert(__iter,__node);

    if(__dom != __node->dom) {
        __node->dom = std::move(__dom);
        update_tag  = true;
    }
}


void dominate_maker::rename(node *__node) {
    /* Already in the set. */
    if(node_set.insert(__node).second == false) return;
    auto *__tmp = new auto(var_map);
    auto &__map = node_phi[__node];

    /* Rename the phi_stmt and build up the dest. */
    for(auto [__var,__phi] : __map) {
        __phi->dest = new IR::temporary;
        __phi->dest->name = string_join(__var->name,".mem.",std::to_string(phi_count++));
        __phi->dest->type = --__var->type;
        var_map[__var].push_back(__phi->dest);
    }

    /* Now the block informations are collected */
    collect_block(__node);

    /* Set the branch for the phi_stmt. */
    for(auto __next : __node->next) {
        update_branch(__node,__next);
        rename(__next);
    }

    var_map = std::move(*__tmp); delete __tmp;
}


void dominate_maker::collect_block(node *__node) {
    std::vector <IR::statement *> __ans;
    for(auto __p : __node->block->stmt) {
        /* Store case. */
        if(auto __store = dynamic_cast <IR::store_stmt *> (__p)) {
            if(auto __var = dynamic_cast <IR::local_variable *> (__store->dst)) {
                var_map[__var].push_back(__store->src);
                continue;
            }
        }

        /* Load case. */
        else if(auto __load = dynamic_cast <IR::load_stmt *> (__p)) {
            if (auto *__var = dynamic_cast <IR::local_variable *> (__load->src)) {
                auto &__vec = var_map[__var];
                IR::definition *__def; /* Current definition. */

                if(__vec.empty()) {
                    warning("Undefined behavior: Load from an uninitialized variable!");
                    __def = create_undefined(__var->get_point_type());
                } else {
                    __def = __vec.back();
                }

                /* Replace the old loaded result with data in stack. */
                auto __iter = use_map.find(__load->dst);
                runtime_assert("WTF is this?",__iter != use_map.end());
                for(auto __p : __iter->second)
                    __p->update(__load->dst,__def);
                /* Now it will be no longer be used. */
                use_map.erase(__iter); continue;
            }
        }

        /* Remove all allocas. */
        else if (dynamic_cast <IR::allocate_stmt *> (__p)) continue;

        /* Normal case. */
        __ans.push_back(__p);
    }

    __node->block->stmt = std::move(__ans);
}


void dominate_maker::update_branch(node *__node,node *__next) {
    /* Just update the phi statement. */
    for(auto [__var,__phi] : node_phi[__next]) {
        auto &__vec = var_map[__var];
        IR::definition *__def = nullptr;
        if(__vec.empty()) {
            __def = create_undefined(__var->get_point_type());
        } else {
            __def = __vec.back();
        }
        __phi->cond.push_back({__def,__node->block});
    }
}

}


/* SSA graph builder. */
namespace dark::OPT {

/* This special impliment will update the CFG graph. */
void SSAbuilder::visitFunction(IR::function *ctx) {
    for(auto __block : ctx->stmt) visitBlock(__block);
}
void SSAbuilder::visitBlock(IR::block_stmt *ctx) {
    top = create_node(ctx);
    ctx->set_impl_ptr(top);
    end_tag = 0;
    auto __beg = ctx->stmt.begin();
    auto __bak = __beg;
    auto __end = ctx->stmt.end();
    while(__beg != __end) {
        if((*__beg)->is_undefined_behavior()) {
            /* Remove unreachable code. */
            ctx->stmt = { IR::create_unreachable() };
            return;
        } else visit(*__beg++);
        /* If terminate, return. */
        if(end_tag) return ctx->stmt.resize(__beg - __bak);
    }
    runtime_assert("Undefined behavior: No terminator in the block!" + ctx->data());
}
void SSAbuilder::visitJump(IR::jump_stmt *ctx) {
    link(top, create_node(ctx->dest));
    end_tag = 1;
}
void SSAbuilder::visitBranch(IR::branch_stmt *ctx) {
    link(top, create_node(ctx->br[0]));
    link(top, create_node(ctx->br[1]));
    end_tag = 1;
}
void SSAbuilder::visitInit(IR::initialization *ctx) {}
void SSAbuilder::visitCompare(IR::compare_stmt *ctx) {}
void SSAbuilder::visitBinary(IR::binary_stmt *ctx) {}
void SSAbuilder::visitCall(IR::call_stmt *ctx) {}
void SSAbuilder::visitLoad(IR::load_stmt *ctx) {}
void SSAbuilder::visitStore(IR::store_stmt *ctx) {}
void SSAbuilder::visitReturn(IR::return_stmt *ctx) { end_tag = 1; }
void SSAbuilder::visitAlloc(IR::allocate_stmt *ctx) {}
void SSAbuilder::visitGet(IR::get_stmt *ctx) {}
void SSAbuilder::visitPhi(IR::phi_stmt *ctx) {}
void SSAbuilder::visitUnreachable(IR::unreachable_stmt *ctx) {}


}