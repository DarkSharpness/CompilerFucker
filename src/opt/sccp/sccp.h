#pragma once

#include "optnode.h"
#include <queue>
#include <unordered_set>
#include <unordered_map>


namespace dark::OPT {

struct constant_propagatior;
struct constant_calculator final : IR::IRvisitorbase {
  protected:
    void *data; /* This argument is responsible for passing the params. */
    using array_type = std::vector<IR::definition *>;
    explicit constant_calculator() = default;

    const array_type &get_array()
    { return *static_cast <const array_type *> (data); }

    explicit operator IR::definition *()
    { return static_cast <IR::definition *> (data); }

    void set_result(IR::definition *__def) 
    { data = static_cast <void *> (__def); }

    friend constant_propagatior;

  public:
 
    /* Work out the const folding result. */
    [[nodiscard]] inline IR::definition *
        work(IR::node *__ctx,const std::vector <IR::definition *> &__vec) {
        data = static_cast <void *> (const_cast <array_type *> (&__vec));
        visit(__ctx);
        return static_cast <IR::definition *> (*this);
    }

    void visitBlock(IR::block_stmt*) override {}
    void visitFunction(IR::function*) override {}
    void visitInit(IR::initialization*) override {}

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
    ~constant_calculator() override = default;
};


/**
 * @brief Sparse conditional constant propagation.
 * It will not change CFG and only do constant propagation.
 * Nevertheless, it still take use of constant branch to
 * better spread constant.
 * 
 */
struct constant_propagatior {
    std::vector <IR::definition *> __cache;
    constant_calculator calc;

    struct usage_info {
        std::vector <IR::node *>  use_list;  /* Use list. */
        IR::definition *new_def  = nullptr;  /* Potential new definition. */
        IR::node       *def_node = nullptr;  /* Definition node. */
    };

    struct block_info {
        std::vector <IR::block_stmt *> visit;/* Visit list. */
        bool visit_from(IR::block_stmt *__prev) {
            if (find(__prev)) return true;
            visit.push_back(__prev);
            return false;
        }

        bool find(IR::block_stmt *__prev) const noexcept {
            for(auto __block : visit) if (__block == __prev) return true;
            return false;
        }

        size_t visit_count() const noexcept { return visit.size(); }
    };

    struct edge_info { IR::block_stmt *prev, *next; };

    std::unordered_map <IR::temporary *, usage_info>   use_map;
    std::unordered_map <IR::block_stmt *,block_info> block_map;
    std::unordered_map <IR::node *,block_info *> node_map;

    using CFG_Container = std::queue <edge_info>;
    using SSA_Container = std::queue <IR::node *>;

    CFG_Container CFG_worklist;
    SSA_Container SSA_worklist;

    constant_propagatior(IR::function *,node *);

    /* A hacking method! */
    inline IR::block_stmt *get_block(IR::node *__node) {
        /* Requires plain memory setting. */
        static_assert(sizeof(*block_map.begin()) == sizeof(void *) + sizeof(block_info));
        void *__info = node_map[__node];
        return *(static_cast <IR::block_stmt **> (__info) - 1);
    }

    void init_info(IR::function *);

    /* Collect all usage information in all may-go-constant nodes. */
    void collect_use(IR::function *);

    /* Judge whether one node can be constant defed. */
    static bool may_go_constant(IR::node *);

    void update_SSA();
    void update_CFG();
    void update_PHI(IR::phi_stmt *);
    void visit_branch(IR::branch_stmt *);
    void visit_node(IR::node *);
    void try_update_value(IR::node *);
    IR::definition *get_value(IR::definition *);
    void update_constant(IR::function *);

};


/**
 * @brief This is helper class that will remove all single phi.
 * It will replace single phi from its source.
 * It will not change the CFG graph structure.
 * 
*/
struct single_killer {
    /* Map of temporary usage. */
    std::unordered_map <IR::temporary *,std::vector <IR::node *>> use_map;

    /* A list of phi statements to remove. */
    std::queue <IR::phi_stmt *> work_list;

    single_killer(IR::function *,node *);

    static IR::definition *merge_phi_value(IR::phi_stmt *__phi);
    void collect_single_phi(IR::function *,node *);
    void remove_single_phi(IR::function *,node *);
};


}