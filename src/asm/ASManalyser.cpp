#include "ASManalyser.h"

#include <algorithm>

namespace dark::ASM {


/* Make the order of blocks in one function. */
void ASMliveanalyser::make_order(function *__func) {
    std::unordered_set <block *> __visited;
    auto && __vec = __func->blocks;
    auto *__entry = __vec[0];
    __vec.clear();
    auto &&__dfs  = [&](auto &&__self, block *__block) -> void {
        if (!__visited.insert(__block).second) return;
        for (auto __next : __block->next) __self(__self, __next);
        __vec.push_back(__block);
    };

    __dfs(__dfs, __entry);

    /* Label them all! */
    std::reverse(__vec.begin(), __vec.end());
    size_t __cnt = 0;
}



}