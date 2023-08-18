#include "ASMallocator.h"


namespace dark::ASM {

ASMallocator::store_info ASMallocator::allocate(virtual_register *__reg) {
    /* If useless left value, it will be discarded (write to zero.) */
    if(!--counter[__reg]) {
        deallocate(__reg);
        return {get_register(register_type::zero)};
    }

    /* Left value doesn't need to be loaded. */
    auto __tmp = access(__reg);
    return {.reg = __tmp.reg,.pos = __tmp.spos};
}

std::vector <ASMallocator::load_info>
    ASMallocator::access(std::vector <virtual_register *> __vec) {
    /* This is used to avoid unnecessary load/store from memory. */
    for(auto *__reg : __vec) activate(__reg);

    /* Load them one by one. */
    std::vector <load_info> __ret;
    __ret.reserve(__vec.size());
    for(auto *__reg : __vec) __ret.push_back(access(__reg));
    for(auto *__reg : __vec) if(!--counter[__reg]) deallocate(__reg);
    return __ret;
}

ASMallocator::load_info ASMallocator::access(virtual_register *__reg) {
    auto __iter   = status_map.find(__reg);
    size_t __load = NPOS;
    if(__iter != status_map.end()) {
        if(__iter->second.is_in_stack())
            __load = __iter->second.index;
        else /* Return the immediate register.  */
            return {.reg = __iter->second.reg};
    }

    auto [__new,__idx] = get_new_register();
    if(__load != NPOS) idx_pool.push_back(__load);

    reg_list.push_back({__new,__reg});
    status_map[__reg] = {.st = register_status::REGISTER, .reg = __new};
    return {.reg = __new,.lpos = __load,.spos = __idx};
}

std::vector <ASMallocator::store_info> ASMallocator::call() {
    std::vector <store_info> __ret;
    __ret.reserve(reg_list.size());
    while(!reg_list.empty()) __ret.push_back(pop_oldest());
    return __ret;
}

void ASMallocator::deallocate(virtual_register *__reg) {
    auto __iter = status_map.find(__reg);
    if (__iter == status_map.end()) return;
    if(__iter->second.is_in_stack())
        idx_pool.push_back(__iter->second.index);
    else {
        reg_list.remove({__iter->second.reg,__reg});
        reg_pool.push_back(__iter->second.reg);
    }
    status_map.erase(__iter);
}



}