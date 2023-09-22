#include "ASManalyser.h"

#include <algorithm>

namespace dark::ASM {


/**
 * @brief Init the save set after it has been built up.
 * No modification should be performed to it afterwards.
 */
void ASMliveanalyser::usage_info::init_save_set() {
    std::sort(save_set.begin(),save_set.end());
}

/**
 * @return Whether the register needs to be saved
 * during the function call (As callee save register).
 */
bool ASMliveanalyser::usage_info::
    need_saving(function_node *__call) const noexcept {
    return std::binary_search(save_set.begin(),save_set.end(),__call);
}

/**
 * @brief Rearrange the order of blocks in one function.
 * 
 * Traditionally, we might opt for reverse post order(RPO).
 * However, we may speed up by visiting non-loop
 * related blocks first in post order traversal.
 * 
 * This type of RPO generates a better code using
 * linear scan register allocation.
 * 
*/
void ASMliveanalyser::make_order(function *__func) {
    std::unordered_set <block *> __visited;
    auto && __vec = __func->blocks;
    __vec.clear();
    auto *__entry = __vec[0];
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

void ASMliveanalyser::init_count(std::vector <block *> &__range) {
    int __count  = 0; /* Count of used variables. */
    std::vector <Register *> __uses_list; /* Cached to speed up. */
    __uses_list.reserve(4);
    for(auto __block : __range) {
        __count += __block->expression.size();
        for(auto &__p : __block->expression) {
            __uses_list.clear();
            __p->get_use(__uses_list);
            for (auto &__q : __uses_list) {
                if (auto __vir = dynamic_cast <virtual_register *> (__q);
                    __vir != nullptr && !__block->def.count(__vir))
                    __block->use.insert(__vir);
            }
            auto __def = __p->get_def();
            if (auto __vir = dynamic_cast <virtual_register *> (__def))
                __block->def.insert(__vir);
        }
        __block->set_impl_val <int> (++__count);
        __block->livein = __block->use;
    }
}
 

ASMliveanalyser::ASMliveanalyser(function *__func) {
    make_order(__func);
    /* Collect data in just one block. */
    auto &&__range  = __func->blocks;
    init_count(__range);

    /* Collect data in all blocks. */
    auto __update = [&](block *__block) -> bool {
        bool __changed = false;
        /* The live-out depends the live-in information.  So use post order. */
        for(auto __next : __block->next) {
            for(auto __vir : __next->livein) {
                if (__block->def.count(__vir)) continue;
                __changed |= __block->liveout.insert(__vir).second;
            }
        }

        /* All uses has been loaded. The only increment comes from liveout. */
        for(auto __vir : __block->liveout) {
            if (__block->def.count(__vir)) continue;
            __changed |= __block->livein.insert(__vir).second;
        }
        return __changed;
    };

    /* Update until no change. */
    do {
        bool changed = false;
        auto __beg = __range.rbegin();
        auto __end = __range.rend();
        while(__beg != __end) changed |= __update(*__beg++);  
        if (!changed) break;
    } while(true);

    struct call_data {
        size_t index;  /* Index of specific command.    */
        double weight; /* Loop weight of this command.  */
    };
    std::vector <call_data > __call_list; /* Call data list. */
    std::vector <Register *> __uses_list; /* Cached to speed up. */
    __uses_list.reserve(4);

    /* Collect all call data and normal data. */
    for(size_t i = 0 ; i < __range.size() ; ++i) {
        auto __block = __range[i];
        auto __count = __block->get_impl_val <int> ();
        auto  __live = __block->liveout; 
        /* Must be live in the exit. */
        for(auto __vir : __live)
            usage_map[__vir].update(i,__count);

        auto __beg = __block->expression.rbegin();
        auto __end = __block->expression.rend();
        const auto __factor = __block->loop_factor;
        while(__beg != __end) {
            /* Basic assignment and calc. */
            auto *__node = *(__beg++); --__count;

            /* Virtual register definition. */
            if (auto __vir = dynamic_cast <virtual_register *> (__node->get_def()))
                usage_map[__vir].update(i,__count) , __live.erase(__vir);

            /* If non-tail call-expression, update info. */
            if (auto __call = dynamic_cast <function_node *> (__node);
                __call && __call->op != __call->TAIL) {
                /* All live across function will bring some cost. */
                for(auto __vir : __live) {
                    auto &__ref = usage_map[__vir];
                    __ref.call_weight += __factor;
                    __ref.save_set.push_back(__call);
                }
            }

            __uses_list.clear();
            __node->get_use(__uses_list);
            for(auto __use : __uses_list) {
                auto __vir = dynamic_cast <virtual_register *> (__use);
                if (!__vir) continue;
                auto &__ref = usage_map[__vir];
                __ref.update(i,__count);
                __ref.use_weight += __factor;
                __live.insert(__vir);
            }
        }
    }

    /* Initialize the save set. This will help further optimization. */
    for(auto &&[__vir,__ref] : usage_map) __ref.init_save_set();
}



}