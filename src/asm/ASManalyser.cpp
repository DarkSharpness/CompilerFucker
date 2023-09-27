#include "ASManalyser.h"

#include <algorithm>

namespace dark::ASM {


/**
 * @brief Init the save set after it has been built up.
 * No modification should be performed to it afterwards.
 */
void ASMlifeanalyser::usage_info::init_set() {
    std::sort(live_set.begin(),live_set.end());
    if (live_set.empty()) return;
    auto __beg = live_set.begin();
    auto __bak = __beg;
    auto __end = live_set.end();
    while(__beg != __end) {
        /* Merge 2 range if possible. */
        if (__beg->first <= __bak->second + 1)
            __bak->second = __beg++ ->second;
        else *(++__bak) = *(__beg++);
    }
    live_set.resize(__bak + 1 - live_set.begin());

    /* Actually, it is used in range [l,r) */
    for(auto &__p : live_set) life_length += __p.second - __p.first;
}

/**
 * @return 0 : dead.    ||
 *         1 : in hole. ||
 *         2 : live. 
 */
int ASMlifeanalyser::usage_info::get_state(int __index) const noexcept {
    if (!life_length) return 0; /* Dead. */
    bool __flag1 = false;
    bool __flag2 = false;

    /* Due to obeservation that live set is small, we don't use binary search. */
    for(auto [__beg,__end] : live_set) {
        if (__index < __beg) __flag2 = true;
        if (__index > __end) __flag1 = true;
        return 2; /* Within the range [__beg,__end]: alive. */
    }

    /* If not both flag is on, then the node is dead! */
    return __flag1 & __flag2;
}


/* Return the begin index of a live variable. */
int ASMlifeanalyser::usage_info::get_begin() const noexcept {
    if (!life_length) return 0;
    return live_set.front().first;
}

/* Return the end index of a live variable. */
int ASMlifeanalyser::usage_info::get_end() const noexcept {
    if (!life_length) return 0;
    return live_set.back().second;
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
void ASMlifeanalyser::make_order(function *__func) {
    std::unordered_set <block *> __visited;
    auto && __vec = __func->blocks;
    for(auto __block : __vec) {
        auto &__next = __block->next;
        /* Branch case. */
        if (__next.size() == 2) {
            auto &__next0 = __next[0];
            auto &__next1 = __next[1];

            /* Always visit prefered/loop node first. */
            if(__block->old_block->prefer == __next1->old_block
            || __next0->loop_factor < __next1->loop_factor)
                std::swap(__next0,__next1);
        }
    }

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

void ASMlifeanalyser::init_count(std::vector <block *> &__range) {
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


ASMlifeanalyser::ASMlifeanalyser(function *__func) {
    make_order(__func);
    /* Collect data in just one block. */
    auto &&__range  = __func->blocks;
    /* Specially, an entry contains dummy use of a0 ~ a7 arguments. */
    [__entry = __range[0],__func]() -> void {
        for(auto __vir : __func->dummy_def)
            __entry->def.insert(
                safe_cast <virtual_register *> (__vir.pointer.reg)
            );
    } ();

    init_count(__range);

    /* Collect data in all blocks. */
    auto __update = [&](block *__block) -> bool {
        bool __changed = false;
        /* The live-out depends the live-in information.  So use post order. */
        for(auto __next : __block->next) {
            for(auto __vir : __next->livein)
                __changed |= __block->liveout.insert(__vir).second;
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
        std::unordered_map <virtual_register *,int> __live;

        /* Must be live in the exit. */
        for(auto __vir : __block->liveout)
            __live[__vir] = __count;


        auto __beg = __block->expression.rbegin();
        auto __end = __block->expression.rend();
        const auto __factor = __block->loop_factor;

        while(__beg != __end) {
            /* Basic assignment and calc. */
            auto *__node = *(__beg++); --__count;

            /* Virtual register definition. */
            if (auto *__vir = dynamic_cast <virtual_register *> (__node->get_def())) {
                /* Update live set if this argument goes dead. */
                auto __iter = __live.find(__vir);
                if (__iter != __live.end()) {
                    usage_map[__vir].live_set.push_back({__count,__iter->second});
                    __live.erase(__iter);
                }
            }

            /* If non-tail call-expression, update info. */
            if (auto __call = dynamic_cast <function_node *> (__node);
                __call && __call->op != __call->TAIL) {
                /* All live across function will bring some cost. */
                for(auto [__vir,___] : __live) {
                    auto &__ref = usage_map[__vir];
                    __ref.call_weight += __factor * 1.5;
                    __ref.save_set.push_back({__call,__count});
                }
            }

            __uses_list.clear();
            __node->get_use(__uses_list);
            for(auto __use : __uses_list) {
                auto __vir = dynamic_cast <virtual_register *> (__use);
                if (!__vir) continue;
                usage_map[__vir].use_weight += __factor;
                /* If dead, update its right index. Else, do nothing. */
                __live.try_emplace(__vir,__count);
            }
        }

        --__count; // Update the count.
        for(auto [__vir,__int] : __live)
            usage_map[__vir].live_set.push_back({__count,__int});
    }

    /* Initialize the save set. This will help further optimization. */
    for(auto &&[__vir,__ref] : usage_map) __ref.init_set();

    // debug_print(std::cerr,__func);
}


void ASMlifeanalyser::debug_print(std::ostream &__os,function *__func) {
    __os << "-----Debug!-----\n\n  Function: " << __func->name << '\n';
    for(auto __block : __func->blocks) {
        __os << "    Block: " << __block->name  << " ||  live_in: ";
        for(auto __vir : __block->livein) __os << __vir->data() << ' ';
        __os << " ||  live_out: ";
        for(auto __vir : __block->liveout) __os << __vir->data() << ' ';
        __os << " || index: " << __block->get_impl_val <int> () << '\n';
    }

    for(auto &&[__vir,__ref] : usage_map) {
        __os << "    Virtual register: " << __vir->data();
        for(auto [__beg,__end] : __ref.live_set)
            __os << "  [" << __beg << ',' << __end << "]";
        __os << '\n';
    }

    __os << "\n-----End Debug!-----\n";
}


}