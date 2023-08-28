#include "IRnode.h"
#include "IRbase.h"
#include "memnode.h"

#include <queue>

/* Optimize only. */
namespace dark::MEM {

/* An interesting helper class. */
struct info_collector {
    static std::set <IR::variable *> collect_def(IR::block_stmt *__block) {
        std::set <IR::variable *> __set;
        for(auto __stmt : __block->stmt)
            if(auto __store = dynamic_cast <IR::store_stmt *> (__stmt))
                if(auto __var = dynamic_cast <IR::local_variable *> (__store->dst))
                    __set.insert(__var);
        return __set;
    }

    static void collect_use(IR::block_stmt *__block,
        std::map <IR::definition *,std::vector <IR::node *>> &__map)  {
        for(auto __stmt : __block->stmt) {
            auto __vec = __stmt->get_use();
            for(auto __use : __vec)
                __map[__use].push_back(__stmt);
        }
    }
};

/* Iterative dominate algorithm */
struct dominate_maker {
    size_t phi_count = 0;

    /* A set of all the nodes. */
    std::set    <node *> node_set;
    std::vector <node *> node_rpo;

    /* Mapping from a node to all the variable definitions. (Store) */
    std::map <node *,std::set <IR::variable *>> node_def;

    /* Mapping from a node to all its newly inserted variable-phi pair. */
    std::map <node *,std::map <IR::variable *,IR::phi_stmt *>> node_phi;

    /* Mapping from a variable to its definition. */
    std::map <IR::variable *,std::vector <IR::definition *>> var_map;

    /* Mapping from a old temporary (loaded) to new definition. */
    std::map <IR::definition *,IR::definition *> use_map;

    /* Mapping from a old temporary to all nodes that use it. */
    std::map <IR::definition *,std::vector <IR::node *>> stmt_map;

    bool update_tag = false;

    dominate_maker(node *);

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
};


struct graph_builder : IR::IRvisitorbase {
    std::map <IR::block_stmt *,node> node_map;
    node *top;
    size_t end_tag = 0;

    node *create_node(IR::block_stmt *__block) {
        return &node_map.try_emplace(__block,__block).first->second;
    }

    graph_builder
        (std::vector <IR::initialization> &global_variables,
         std::vector <IR::function   >    &global_functions) {
        for(auto __func : global_functions)
            visitFunction(&__func);

        // for(auto __func : global_functions)
        //      debug_print(&__func);

        for(auto __func : global_functions)
            dominate_maker __maker(create_node(__func.stmt.front()));
    }

    void visitBlock(IR::block_stmt*) override;
    void visitFunction(IR::function*) override;
    void visitInit(IR::initialization*) override;

    void visitCompare(IR::compare_stmt *) override;
    void visitBinary(IR::binary_stmt *) override;
    void visitJump(IR::jump_stmt *) override;
    void visitBranch(IR::branch_stmt *) override;
    void visitCall(IR::call_stmt *) override;
    void visitLoad(IR::load_stmt *) override;
    void visitStore(IR::store_stmt *) override;
    void visitReturn(IR::return_stmt *) override;
    void visitAlloc(IR::allocate_stmt *) override;
    void visitGet(IR::get_stmt *) override;
    void visitPhi(IR::phi_stmt *) override;
    void visitUnreachable(IR::unreachable_stmt *) override;

    inline void link(node *from, node *to) {
        from->next.push_back(to);
        to->prev.push_back(from);
    }

    void debug_print(IR::function *ctx,std::ostream &os = std::cerr) {
        os << "Function: " << ctx->name << "\n";
        os << "  Blocks:\n";
        for(auto __p : ctx->stmt)
            os << create_node(__p)->data() << '\n';
    }

    virtual ~graph_builder() = default;
};



}
