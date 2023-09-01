#pragma once
#include "IRnode.h"

#include <set>
#include <vector>

/* Optimizations. */
namespace dark::OPT {

struct node {
    IR::block_stmt *block;

    std::vector <node *> prev;
    std::vector <node *> next;

    explicit node(IR::block_stmt *__block) : block(__block) {}

    const std::string &name() const noexcept { return block->label; }

    std::vector <node *> dom; /* Dominated dots. */
    std::vector <node *> fro; /* Dominated Frontier. */

    bool is_entry() const noexcept { return prev.empty(); }
    bool is_exit()  const noexcept { return next.empty(); }

    std::string data() const noexcept {
        std::vector <std::string> buf;
        buf.reserve(1 + next.size());
        buf.push_back(string_join("    Current block: ",name(),"    \n"));
        for(auto __p : next)
            buf.push_back(string_join("    Next block: ",__p->name(),"    \n"));
        return string_join_array(buf.begin(),buf.end());
    }
};


/* An interesting helper class. */
struct info_collector {
    static void collect_def(IR::block_stmt *__block,
        std::set <IR::variable *> &__set) {
        for(auto __stmt : __block->stmt)
            if(auto __store = dynamic_cast <IR::store_stmt *> (__stmt))
                if(auto __var = dynamic_cast <IR::local_variable *> (__store->dst))
                    __set.insert(__var);
    }

    static void collect_use(IR::block_stmt *__block,
        std::map <IR::definition *,std::vector <IR::node *>> &__map)  {
        for(auto __stmt : __block->stmt) {
            auto __vec = __stmt->get_use();
            for(auto __use : __vec)
                if(dynamic_cast <IR::non_literal *> (__use))
                    __map[__use].push_back(__stmt);
        }
    }
};
    

}