#include "ASMbuilder.h"
#include "IRbasic.h"

namespace dark::ASM {

void ASMbuilder::build_rodata() {
    for(auto &[__name,__var] : IR::IRbasic::string_map) {
        auto __str = safe_cast <IR::string_constant *> (__var->const_val);
        global_info.rodata_list.push_back({__var,__str});
    }
}


}