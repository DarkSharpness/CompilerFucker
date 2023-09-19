#include "ASMnode.h"


namespace dark::ASM {

std::string stack_address::data() const {
    runtime_assert("Not implemented!");
    return "";
}

std::string ret_expression::data() const {
    runtime_assert("Not implemented!");
    return "";
}

void global_information::print(std::ostream &__os) const {
    runtime_assert("Not implemented!");
}

std::string call_function::data() const {
    runtime_assert("Not implemented!");
    return "";
}

}