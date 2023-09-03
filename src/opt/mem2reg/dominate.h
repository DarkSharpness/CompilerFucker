#pragma once

#include "optnode.h"


namespace dark::OPT {

/* Iterative dominate algorithm */
struct dominate_maker {
    size_t phi_count = 0;

    /* A set of all the nodes. */
    std::set    <node *> node_set;
    std::vector <node *> node_rpo;

    /* Mapping from a node to all its newly inserted variable-phi pair. */
    std::map <node *,std::map <IR::variable *,IR::phi_stmt *>> node_phi;

    /* Mapping from a variable to its definition. */
    std::map <IR::variable *,std::vector <IR::definition *>> var_map;

    /* Mapping from a old temporary to all nodes that use it. */
    std::map <IR::definition *,std::vector <IR::node *>> use_map;

    bool update_tag = false;

    dominate_maker(IR::function *,node *);

    /* Work out the RPO and collect all nodes. */
    void dfs(node *);

    /* Work out dominant by iteration. */
    void iterate(node *);

    /* The real function that helps iteration. */
    void update(node *);

    /* Spread a node's definition to its frontier. */
    void spread_def(node *,std::set <IR::variable *> &);

    /* Spread a node's phi-defs to its frontier. */
    void spread_phi();

    /* Renaming for a node. */
    void rename(node *);

    /* Update node's branch. */
    void update_branch(node *,node *);

    /* Update var_map for the node and build up use_map */
    void collect_block(node *);

    /* Debug use only */
    void debug_print(std::ostream &os) {
        for(auto __p : node_rpo) {
            os << __p->name() << ":";
            for(auto __q : __p->fro)
                os << ' ' << __q->name();
            os << '\n';
        }
        os << '\n';
    }

    /* An interesting helper class. */
    struct info_collector {
        /* Collect all definition of local variables (Store only) */
        static void collect_def(IR::block_stmt *__block,
            std::set <IR::variable *> &__set) {
            for(auto __stmt : __block->stmt)
                if(auto __store = dynamic_cast <IR::store_stmt *> (__stmt))
                    if(auto __var = dynamic_cast <IR::local_variable *> (__store->dst))
                        __set.insert(__var);
        }

        /* Collect all usage of temporaries(which may be loaded from variables). */
        static void collect_use(IR::block_stmt *__block,
            std::map <IR::definition *,std::vector <IR::node *>> &__map)  {
            for(auto __stmt : __block->stmt) {
                auto __vec = __stmt->get_use();
                for(auto __use : __vec)
                    if(dynamic_cast <IR::temporary *> (__use))
                        __map[__use].push_back(__stmt);
            }
        }
    };
};

}
