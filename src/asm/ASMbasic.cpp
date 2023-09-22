#include "ASMnode.h"


namespace dark::ASM {

std::string stack_address::data() const {
    warning("Not implemented!");
    return "";
}

std::string ret_expression::data() const {
    std::string __prefix;

    /* Need to move to target register. */
    if (rval && rval != get_physical(10))
        __prefix = string_join(__indent, "mv a0, ", rval->data(), '\n');

    return string_join(__prefix,func->return_data(),__indent, "ret");
}

std::string call_function::data() const {
    if (op == TAIL) {
        /* No calling convention worry. Simplified to a jump. */
        if (self == func)
            return string_join(__indent, "j ", func->blocks[0]->name);
        else
            return string_join(self->return_data(),__indent, "tail ", func->name);
    }

    std::string __suffix;
    /* Need to move to target register. */
    if (dest && dest != get_physical(10))
        __suffix = string_join('\n', __indent, "mv ", dest->data(), ", a0");

    return string_join(__indent,"call ",func->name, __suffix);
}

void global_information::print(std::ostream &__os) const {
    __os << "    .section .text\n";
    for(auto __p : function_list) __p->print(__os);

    __os << '\n';
    __os << "    .section .data\n";
    for(auto [__var,__lit] : data_list) {
        __os << "    .globl " << __var->name << '\n';
        __os << __var->name << ":\n";
        auto __name = __lit->data();
        if(__name == "null") __name = "0    # 0x00";
        __os << "    .word " << __name << '\n';
    }

    __os << '\n';
    __os << "    .section .rodata\n";
    for(auto [__var,__lit] : rodata_list) {
        __os << "    .globl " << __var->name << '\n';
        __os << __var->name << ":\n";
        __os << "    .asciz " 
             << safe_cast <IR::string_constant *> (__lit)->ASMdata() << "\n";
    }
}

void function::print(std::ostream &__os) const {
    __os << __indent << ".globl " << name << '\n';
    __os << name << ":\n";
    __os << "# " << name << ".prework:\n";

    size_t n = 0;
    /* Save those callee save registers. */
    for(auto __tmp : callee_save)
        __os << __indent << "sw "
             << __tmp->data() << ", -" << (++n) * 4 << "(sp)\n";

    /* Move the stack pointer. */
    n = get_stack_space();
    __os << __indent << "addi sp, sp, -" << n << "\n\n";

    for(auto __block : blocks) __os << __block->data() << '\n';
    __os << '\n';
}


/* Things to do before returning. */
std::string function::return_data() const {
    std::vector <std::string> __buf;
    __buf.reserve(callee_save.size() + 1);

    /* Restore the stack pointer. */
    size_t n = get_stack_space();
    __buf.push_back(string_join(__indent, "addi sp, sp, ", std::to_string(n),'\n'));

    /* Save those callee save registers. */
    n = 0;
    for(auto __tmp : callee_save)
        __buf.push_back(string_join(__indent, "lw ",
            __tmp->data(), ", -", std::to_string((++n) * 4), "(sp)\n"));

    return string_join_array(__buf.begin(),__buf.end());
}

size_t function::get_stack_space() const {
    size_t __n = arg_offset + var_count + (stk_count + callee_save.size()) * 4;
    return (__n + 15) / 16 * 16;
}




}