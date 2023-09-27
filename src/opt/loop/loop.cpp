#include "loop.h"
#include "dominate.h"
#include <queue>

#include <algorithm>

namespace dark::OPT {

loop_detector::loop_detector(IR::function *__func,node *__entry) {
    dominate_maker {__entry,true};
    std::cerr << __func->name << '\n';

    /* First, find all backward edges. */
    for(auto __block : __func->stmt) {
        __block->loop_factor = 1;
        auto __node = __block->get_impl_ptr <node> ();
        auto &__dom = __node->dom;
        for(auto __next : __node->next) {
            /* A backward edge: Loop detected! */
            if (std::binary_search(__dom.begin(),__dom.end(),__next)) {
                auto &__ref  = loop_map[__next];
                __ref.header = __next;
                __ref.loop_blocks.insert(__node);
            }
        }
    }


    /* Find all node in the tree. */
    auto &&__bfs = [](loop_info *__loop) -> void {
        std::queue <node *> work_list;
        for(auto __block : __loop->loop_blocks)
            work_list.push(__block);
        while(!work_list.empty()) {
            auto *__node = work_list.front(); work_list.pop();
            if (__node == __loop->header) continue;
            for(auto __prev : __node->prev)
                if (__loop->loop_blocks.insert(__prev).second)
                    work_list.push(__prev);
        }
        /* Debug print part. */
        std::cerr << "Loop header: " << __loop->header->name() << std::endl;
        for(auto __block : __loop->loop_blocks)
            std::cerr << "  Loop block: " << __block->name() << std::endl;
        std::cerr << "--End of loop!" << std::endl << std::endl;
    };

    for(auto &[__header,__loop] : loop_map) __bfs(&__loop);

    std::vector         <node *> node_rpo;
    std::unordered_set  <node *> visited;

    /* Find the RPO of the CFG with loop optmization. */
    auto &&__dfs = [&](auto &&__self,node *__node) -> void {
        if (!visited.insert(__node).second) return;
        switch(__node->next.size()) {
            case 1: __self(__self,__node->next[0]);
            case 0: return static_cast <void> (node_rpo.push_back(__node));
        }
        auto __iter = loop_map.find(__node);
        /* Always visit blocks in the same loop first! */
        if (__iter != loop_map.end()) {
            auto &__set = __iter->second.loop_blocks;
            bool __flag = __set.count(__node->next[0]);
            if (__flag) __node->block->prefer = __node->next[0]->block;
            if (__set.count(__node->next[1])) {
                if (!__flag) {
                    std::swap(__node->next[0],__node->next[1]);
                    __node->block->prefer = __node->next[0]->block;
                } else {
                    __node->block->prefer = nullptr;
                }
            }
        }
        __self(__self,__node->next[0]);
        __self(__self,__node->next[1]);
        node_rpo.push_back(__node);
    };

    __dfs(__dfs,__entry);

    /* Nothing special. */
    for(auto &[__header,__loop] : loop_map) {
        for(auto __node : __loop.loop_blocks) {
            __node->block->loop_factor *= 10;
        }
    }

}




}