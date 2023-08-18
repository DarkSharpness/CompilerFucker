#pragma once
#include "ASMnode.h"

#include <list>

namespace dark::ASM {

struct ASMallocator : register_allocator {
    struct register_status {
        enum {
            IN_STACK,
            REGISTER,
        } st;
        union { /* Either one of them works. */
            size_t index; /* Index in the range. */
            physical_register *reg; /* Register */
        };
        bool is_in_stack() const { return st == IN_STACK; }
        bool is_register() const { return st == REGISTER; }
    };

    /* Status of all the virtual registers inside. */
    std::map <virtual_register *, register_status> status_map;
    /* Counter of the virtual register. */
    std::map <virtual_register *, size_t> counter;

    /* A list-based LRU. Remove the oldest virtual register when required. */
    std::list <std::pair <physical_register *,virtual_register *>> reg_list;
    std::vector <physical_register *> reg_pool; /* Available registers. */
    std::vector <size_t>              idx_pool; /* Available indexs. */
    size_t idx_counter; /* Counter of maximum allocated space. */

    store_info allocate(virtual_register *) override;
    /* Access one element and return its load information. */
    load_info access(virtual_register *);
    std::vector <load_info> access(std::vector <virtual_register *>) override;
    std::vector <store_info> call() override;
    void deallocate(virtual_register *)  override;
    size_t max_size() const override { return idx_counter; }

    /* Get a new index. */
    size_t get_new_index() {
        if(idx_pool.empty()) return idx_counter++;
        else {
            size_t __ans = idx_pool.back();
            idx_pool.pop_back();
            return __ans;
        }
    }

    /* Remove the oldest unused register from list to stack.  */
    store_info pop_oldest() {
        runtime_assert("No register available!",!reg_list.empty());
        auto __pair = reg_list.front();
        reg_list.pop_front();
        /* This is the register allocated. */

        /* Modify the data of the old register. */
        auto  &__st = status_map.at(__pair.second);
        /* Some small assertion. */
        runtime_assert("Must in register!",
            __st.is_register(),
            __st.reg == __pair.first
        );

        __st.st    = __st.IN_STACK;
        __st.index = get_new_index();
        return {.reg = __pair.first ,.pos = __st.index};
    }

    /* Get a free physical register. */
    store_info get_new_register() {
        if(reg_pool.empty()) {
            return pop_oldest();
        } else { /* Take out from the pool. */
            auto *__new = reg_pool.back();
            reg_pool.pop_back();
            return {.reg = __new};
        }
    }

    /* Tries to move the position in LRU to the back. */
    void activate(virtual_register *__reg) {
        for(auto iter = reg_list.begin(); iter != reg_list.end(); ++iter)
            if(iter->second == __reg)
                return reg_list.splice(reg_list.end(),reg_list,iter);
    }


    ASMallocator() : reg_pool({
        get_register(register_type::t0),
        get_register(register_type::t1),
        get_register(register_type::t2),
        get_register(register_type::t3),
        get_register(register_type::t4),
        get_register(register_type::t5),
        get_register(register_type::t6)
    }), idx_counter(0) {}

    ~ASMallocator() override = default;


};


}
