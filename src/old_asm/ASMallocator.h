#pragma once
#include "ASMnode.h"

#include <list>

namespace dark::ASM {

struct ASMallocator : register_allocator {

    /* Status of all the virtual registers inside. */
    std::map <virtual_register *, size_t> status_map;

    /* Pool of useable registers. */
    std::vector <physical_register *> reg_pool;

    store_info allocate(virtual_register *) override;

    std::vector <load_info> access(std::vector <virtual_register *>) override;
    std::vector <store_info> call();
    void deallocate(virtual_register *) override;
    size_t max_size() const override { return status_map.size(); }

    physical_register *get_new_register() {
        static size_t index = -1;
        if(++index == reg_pool.size()) index = 0;
        return reg_pool[index];
    }

    ASMallocator() : reg_pool({
        get_register(register_type::t0),
        get_register(register_type::t1),
        get_register(register_type::t2),
        get_register(register_type::t3),
        get_register(register_type::t4),
        get_register(register_type::t5),
        get_register(register_type::t6)
    }) {}

    ~ASMallocator() override = default;


};


}
