#include "IRnode.h"
#include "IRbase.h"
#include "memnode.h"


/* Optimize only. */
namespace dark::MEM {

/* An interesting helper class. */
struct info_collector {
    static std::set <IR::variable *> collect_def(IR::block_stmt *__block) {
        std::set <IR::variable *> __set;
        for(auto __stmt : __block->stmt)
            if(auto __store = dynamic_cast <IR::store_stmt *> (__stmt))
                if(auto __var = dynamic_cast <IR::variable *> (__store->dst))
                    if(!dynamic_cast <IR::function_argument *> (__var))
                        __set.insert(__var);
        return __set;
    }
    static std::set <IR::variable *> collect_use(IR::block_stmt *);
};

/* Iterative dominate algorithm */
struct dominate_maker {
    /* A set of all the nodes. */
    std::set    <node *> node_set;
    std::vector <node *> node_rpo;

    std::map <node *,std::set <IR::variable *>> node_def;
    std::map <node *,std::map <IR::variable *,IR::phi_stmt *>> node_phi;

    bool update_tag = false;

    dominate_maker(node *);
    
    void iterate(node *);
    /* Work out the RPO and collect all nodes. */
    void dfs(node *);
    void update(node *);

    /* Spread a node's definition to its frontier. */
    void spread_def(node *,std::set <IR::variable *> &);

    /* Spread a node's phi-defs to its frontier. */
    void spread_phi();

    void debug_print(std::ostream &os) {
        for(auto __p : node_rpo) {
            os << __p->name() << " :";
            for(auto __q : __p->fro)
                os << ' ' << __q->name();
            os << '\n';
        }
    }

};


struct graph_builder : IR::IRvisitorbase {
    std::map <IR::block_stmt *,node> node_map;
    node *top;

    node *create_node(IR::block_stmt *__block) {
        return &node_map.try_emplace(__block,__block).first->second;
    }

    graph_builder
        (std::vector <IR::initialization> &global_variable,
         std::vector <IR::function   >    &global_function) {
        for(auto __func : global_function)
            visitFunction(&__func);
        for(auto __func : global_function)
            debug_print(&__func);
        for(auto __func : global_function) {
            dominate_maker __maker(create_node(__func.stmt.front()));
            __maker;
        }

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


    virtual ~graph_builder() = default;

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
};


}
