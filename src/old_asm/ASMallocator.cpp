#include "ASMallocator.h"


namespace dark::ASM {

ASMallocator::store_info ASMallocator::allocate(virtual_register *__reg) {
    status_map.try_emplace(__reg,status_map.size());
    return {.reg = get_new_register(),.pos = status_map[__reg]};
}


std::vector <ASMallocator::load_info>
    ASMallocator::access(std::vector <virtual_register *> __vec) {
    for(auto __p : __vec) status_map.try_emplace(__p,status_map.size());
    std::vector <load_info> __ret;
    __ret.reserve(__vec.size());
    for(auto __p : __vec)
        __ret.push_back({.reg = get_new_register(),.lpos = status_map[__p]});
    return __ret;
}


std::vector <ASMallocator::store_info> ASMallocator::call() {
    /* Do nothing. */
    return {};
}


void ASMallocator::deallocate(virtual_register *__reg) {
    /* Do nothing. */
}


}