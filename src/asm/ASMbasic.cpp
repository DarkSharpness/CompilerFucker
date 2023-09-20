#include "ASMnode.h"


namespace dark::ASM {

std::string stack_address::data() const {
    warning("Not implemented!");
    return "";
}

std::string ret_expression::data() const {
    if (auto __reg = dynamic_cast <physical_register *> (rval);
        !rval || (__reg && __reg->index == 10))
        return string_join(__indent, "ret");
    return string_join(__indent,
        "mv a0 ", rval->data(), "\n",
        __indent, "ret"
    );
}

std::string call_function::data() const {
    std::string __suffix;
    if (dest && dest != get_physical(10))
        __suffix = string_join('\n', __indent, "mv ", dest->data(), " a0");

    return string_join(__indent,"call ",func->name, __suffix);
}

void global_information::print(std::ostream &__os) const {
    for(auto *__func : function_list)
        __func->print(__os);
}

void function::print(std::ostream &__os) const {
    warning("Not implemented!");
    for(auto __block : blocks)
        std::cerr << __block->data() << std::endl;
}

}