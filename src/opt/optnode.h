#pragma once

#include "optmacro.h"
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

}