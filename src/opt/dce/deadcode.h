#pragma once

#include "optnode.h"
#include "mem2reg.h"
#include "dominate.h"
#include <stack>
#include <unordered_map>

namespace dark::OPT {


/**
 * @brief Eliminator for dead code in a function.
 * It will not only perform dead code elemination, but also
 * greatly simply the CFG by removing useless blocks.
 * 
 */
struct deadcode_eliminator {
    struct info_holder {
        struct info_node {
            info_node *next;
            info_node *prev;
            IR::node  *node;
        };
        /**
         * The special header node.
         * It contains the use-def chain of the temporary.
         * In addition, it also contains the def node.
         * 
        */
        info_node header = {
            .next = &header,
            .prev = &header,
            .node = nullptr
        };

        /**
         * This is a array of all real usage data.
         * It takes a consecutive memory space.
         * 
        */
        struct {
            info_node *array_head = nullptr;
            info_node *array_tail = nullptr;
        };

        /* Insert a node to the holder. */
        void insert(info_node *__node) {
            (__node->next = header.next)->prev = __node;
            (header.next  =    __node  )->prev = &header;
        }

        /* Init with a the definition node and all usage information. */
        void init(IR::node *__node,const std::vector <info_holder *> &__vec) {
            /* Should only call once! */
            runtime_assert("Invalid SSA form!",header.node == nullptr);

            /* Init the data. */
            header.node = __node;
            array_head  = new info_node[__vec.size()];
            array_tail  = array_head + __vec.size();

            /* Link the node into those usage data. */
            auto * __head = array_head;
            for (auto *__info : __vec) {
                (__head++)->node = __node;
                __info->insert(__head);
            }
        }

        /**
         * @brief Delink all the usage data within.
         * It will not delete the memory space.
         * 
        */
        void remove_use() const {
            auto *__head  = array_head;
            while(__head != array_tail) {
                auto *__prev = __head->prev;
                auto *__next = __head->next;
                __prev->next = __next;
                __next->prev = __prev;
                ++__head;
            }
        }

        /* Clear memory storage. */
        ~info_holder() noexcept { delete []array_head; }
    };

    /* Mapping from defs to uses and real node. */
    std::unordered_map <IR::temporary *, info_holder *> info_map;
    /* This is the real data holder. */
    std::stack <info_holder> info_list;

    deadcode_eliminator(IR::function *__func,dominate_maker &__maker);


    /**
     * @brief Get the info tied to the temporary.
     * @param __temp The temporary (Non-nullptr)!
     * @return The info holder tied to the temporary.
     */
    info_holder *get_info(IR::temporary *__temp) {
        auto *&__ptr = info_map[__temp];
        if(!__ptr) __ptr = &info_list.emplace();
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
        return __def ? get_info(__def) : &info_list.emplace();
    }

};


}