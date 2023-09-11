#include "deadcode.h"


namespace dark::OPT {

/* Specially, we will replace all undefed variables as undefined. */
void DCE_base::replace_undefined() {
    for(auto &__info : info_list)
        for(auto &__use : __info.uses)
            if(!info_map.count(__use))
                __info.data->update(__use,IR::create_undefined(__use->type));
}


/* Remove those useless commands. */
void DCE_base::remove_useless(IR::function *__func) {
    std::unordered_set <IR::node *> __set;
    for(auto &__info : info_list)
        if(__info.removable) __set.insert(__info.data);
    std::vector <IR::node *> __vec;
    for(auto __block : __func->stmt) {
        for(auto __stmt : __block->stmt)
            if(!__set.count(__stmt))
                __vec.push_back(__stmt);
        __vec.swap(__block->stmt);
        __vec.clear();
    }
}


}