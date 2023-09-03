#pragma once

#include "optnode.h"
#include "mem2reg.h"
#include "dominate.h"
#include <stack>
#include <queue>
#include <unordered_set>
#include <unordered_map>

namespace dark::OPT {

/**
 * @brief Eliminator for dead code in a function.
 * It will not only perform dead code elemination, but also
 * greatly simply the CFG by removing useless blocks.
 * 
 */
struct deadcode_eliminator {
    struct info_node {
        info_node *next;
        info_node *prev;
        IR::node  *data;
    };

    struct info_holder : info_node {
        info_holder() noexcept : info_node {
            .next = this,
            .prev = this,
            .data = nullptr
        } { /* Safely initialize.*/ }

        /**
         * This is a array of all real usage data.
         * It takes a consecutive memory space.
         * 
        */
        struct {
            info_node *array_head = nullptr;
            info_node *array_tail = nullptr;
        };

        /* Whether the node is removable. */
        bool removable = true;

        inline static constexpr size_t STORE_TAG = 0b01;
        inline static constexpr size_t CALL_TAG  = 0b10;

        /* Insert a node to the holder. */
        void insert(info_node *__node) {
            (__node->next = next)->prev = __node;
            (next   =   __node  )->prev = this;
        }

        /* Init with a the definition node and all usage information. */
        void init(IR::node *__node,const std::vector <info_holder *> &__vec) {
            /* Should only call once! */
            runtime_assert("Invalid SSA form!",data == nullptr);

            /* Init the data. */
            data = __node;
            array_head  = new info_node[__vec.size()];
            array_tail  = array_head + __vec.size();

            removable =
                !dynamic_cast <IR::store_stmt *>  (__node) &&
                !dynamic_cast <IR::call_stmt *>   (__node) &&
                !dynamic_cast <IR::branch_stmt *> (__node) && 
                !dynamic_cast <IR::return_stmt *> (__node) &&
                !dynamic_cast <IR::jump_stmt *>   (__node);

            /* Link the node into those usage data. */
            auto * __head = array_head;
            for (auto *__info : __vec) {
                __head->data = __node;
                __info->insert(__head++);
            }
        }

        info_node *begin() const { return array_head; }
        info_node *end()   const { return array_tail; }

        /* Clear memory storage. */
        ~info_holder() noexcept { delete []array_head; }
    };

    /* Mapping from defs to uses and real node. */
    std::unordered_map <IR::temporary *, info_holder *> info_map;
    /* This is the real data holder. */
    std::deque <info_holder> info_list;
    /* This is the list of variables to remove. */
    std::queue <info_holder *> work_list;

    deadcode_eliminator(IR::function *__func,dominate_maker &__maker);

    void spread_side_fx(node *);

    /**
     * @brief Get the info tied to the temporary.
     * @param __temp The temporary (Non-nullptr)!
     * @return The info holder tied to the temporary.
     */
    info_holder *get_info(IR::temporary *__temp) {
        auto *&__ptr = info_map[__temp];
        if(!__ptr) __ptr = &info_list.emplace_back();
        return __ptr;
    }


    /**
     * @brief 
     * 
     * @param __temp The temporary defined by the node in SSA.
     * @return The info holder tied to the temporary.
     */
    info_holder *create_info(IR::node *__node) {
        auto *__def  = __node->get_def();
        return __def ? get_info(__def) : &info_list.emplace_back();
    }

    /**
     * @brief Return a node from def/use list.
     * @param __info Info that is going to be removed.
     */
    void remove_info(info_holder *__info) {
        for(auto &__temp : *__info) {
            auto *__prev = __temp.prev;
            auto *__next = __temp.next;
            __prev->next = __next;
            __next->prev = __prev;
            /* If this is the last node. */
            if(__next == __prev) 
                work_list.push(static_cast <info_holder *> (__next));
        }
    }

};


}