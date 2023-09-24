#include "ASMnode.h"

#include <queue>

// #define IGNORE_UNUSED(x)
#define IGNORE_UNUSED(x) if (x) return "    # Useless expression!";

#define DEBUG_SHADOW(x) x
// #define DEBUG_SHADOW(x)

namespace dark::ASM {


ssize_t function::get_stack_pos(ssize_t __n) const noexcept {
    return arg_offset + var_count + __n * 4;
}

std::string stack_address::data() const {
    return std::to_string(func->get_stack_pos(index)) + "(sp)";
}

size_t function::get_stack_space() const {
    size_t __n = arg_offset + var_count + (stk_count + callee_save.size() + save_ra) * 4;
    return (__n + 15) / 16 * 16;
}

std::string arith_register::data() const {
    auto __zero = get_physical(0);

    /* Useless case. */
    IGNORE_UNUSED(dest == __zero  || (
        op == ADD &&
               ((dest == lval && rval == __zero) 
            ||  (dest == rval && lval == __zero))))  

    return string_join(__indent, str[op],' ',dest->data(),
        ", ", lval->data(),", ", rval->data()
    );
}

std::string arith_immediat::data() const {
    auto __zero = get_physical(0);
    /* Useless case. */
    IGNORE_UNUSED(dest == __zero || (op == ADD && rval == 0 && dest == lval))

    return string_join(__indent, str[op],"i ",dest->data(),
        ", ", lval->data(),", ", std::to_string(rval)
    );
}

std::string move_register::data() const {
    /* Useless case. */
    IGNORE_UNUSED(dest == get_physical(0) || (dest == from))

    return string_join(__indent,
        "mv ", dest->data(), ", ", from->data()
    );
}

std::string slt_register::data() const {
    /* Useless case. */
    IGNORE_UNUSED(dest == get_physical(0))

    return string_join(__indent,
        "slt ", dest->data(), ", ", lval->data(), ", ", rval->data()
    );
}

std::string slt_immediat::data() const {
    /* Useless case. */
    IGNORE_UNUSED(dest == get_physical(0))

    return string_join(__indent,
        "slti ", dest->data(), ", ", lval->data(), ", ", std::to_string(rval)
    );
}

std::string bool_convert::data() const {
    IGNORE_UNUSED(dest == get_physical(0))
    return string_join(__indent,
        str[op], ' ', dest->data(), ", ", from->data()
    );
}

std::string bool_not::data() const {
    IGNORE_UNUSED(dest == get_physical(0))
    return string_join(__indent,
        "seqz ", dest->data(), ", ", from->data()
    );
}

std::string load_symbol::data() const {
    IGNORE_UNUSED(dest == get_physical(0))
    if (op == HIGH) {
        return string_join(__indent,
            "lui ", dest->data(), ", %hi(", var->name, ")"
        );
    } else if (op == FULL) {
        return string_join(__indent,
            "la ", dest->data(), ", ", var->name
        );
    } else throw std::runtime_error("Not implemented!");
}

std::string load_memory::data() const {
    IGNORE_UNUSED(dest == get_physical(0))
    return string_join(__indent,
        str[op] , ' ', dest->data(), ", ", addr.data()
    );
}

std::string store_memory::data() const {
    return string_join(__indent,
        str[op] , ' ', from->data(), ", ", addr.data()
    );
}

std::string branch_expression::data() const  {
    return string_join(__indent,
        str[op], ' ', lvar->data(), ", ", rvar->data(), ", ", bran[0]->name,
        '\n', __indent, "j ", bran[1]->name
    );
}

std::string ret_expression::data() const {
    std::string __prefix;

    /* Need to move to target register. */
    if (rval && rval != get_physical(10))
        __prefix = string_join(__indent, "mv a0, ", rval->data(), '\n');

    return string_join(__prefix,func->return_data(),__indent, "ret");
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
        if(__name == "null") __name = "0    # 0x00 (nullptr)";
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

std::string function_node::resolve_spill() const {
    std::vector <std::string> __buf;
    __buf.reserve(caller_save.size() * 2 | 1);
    __buf.push_back("    # Storing caller save registers.\n");
    store_memory __store { memory_base::WORD, 0, stack_address {self} };

    /* First, resolve calling convention. */
    for(auto [__reg,__idx] : caller_save) {
        __store.from             = __reg;
        __store.addr.stack.index = __idx;
        __buf.push_back(__store.data());
        __buf.push_back(" # 4-Byte caller save spill.\n");
    }

    return string_join_array(__buf.begin(),__buf.end());
}

std::string function_node::resolve_load() const {
    std::vector <std::string> __buf;
    __buf.reserve(caller_save.size() * 2 | 1);
    __buf.push_back("    # Reloading caller save registers.\n");
    load_memory __load { memory_base::WORD, 0, stack_address {self} };

    /* First, resolve calling convention. */
    for(auto [__reg,__idx] : caller_save) {
        __load.dest             = __reg;
        __load.addr.stack.index = __idx;
        __buf.push_back(__load.data());
        __buf.push_back(" # 4-Byte caller save reload.\n");
    }

    return string_join_array(__buf.begin(),__buf.end());
}


/**
 * Resolve the argument dependency of a function call.
 * This should be used after calling convention is applied.
 * a.k.a: all caller save registers are already saved.
 * 
*/
std::string function_node::resolve_arg() const {
    std::vector <std::string> __buf;
    __buf.reserve(dummy_use.size() * 3);

    struct node_data {
        physical_register *in {nullptr};
        ssize_t           off    {0};
        ssize_t           out    {0};
    };

    /* Lazy assign from global / constant register. */
    std::vector <std::pair <physical_register *,value_type>> lazy_assign;
    /* Lazy load from a fixed address (stack position). */
    std::vector <std::pair <physical_register *,value_type>> lazy_load;
    /* Mapping from physical register to its inner data. */
    std::unordered_map <physical_register *, node_data> __map;

    /* Then, start to resolve dummy use. */
    size_t __top = 0;
    for(auto __val : dummy_use) {
        /* Filter out the dead arguments. */
        while (func->func_ptr->args[__top]->is_dead()) ++__top;
        auto *__def = get_physical(10 + (__top++));
        if (__val.type == value_type::POINTER) {
            auto __use = safe_cast <physical_register *> (__val.pointer.reg);
            /* No operation. */
            if (__def == __use) { __buf.push_back("    # Merged!\n");continue; }
            /* Constantly evaluated case: the source will not be changed. */
            if (__use->index < __use->a0 || __use->index > __use->a7)
                lazy_assign.push_back({__def,__val});
            else {
                __map[__use].out++;
                __map[__def].in  = __use;
                __map[__def].off = __val.pointer.offset;
            }
        } else if (__val.type == value_type::STACK) {
            lazy_load.push_back({__def,__val}); /* Load case. */
        } else lazy_assign.push_back({__def,__val});  /* Global case. */
    }

    /* Finding out the cycles. */
    [&]() {
        std::queue <physical_register *> __work_list;
        for(auto &[__reg,__dat] : __map) if (!__dat.out) __work_list.push(__reg);

        arith_immediat __addi {arith_base::ADD, 0, 0, 0};
        while(!__work_list.empty()) {
            auto *__def = __work_list.front(); __work_list.pop();
            auto __iter = __map.find(__def);
            auto *__in  = __iter->second.in;
            auto  __off = __iter->second.off;
            __map.erase(__iter);
            if (__in == nullptr) continue;

            __addi.lval = __in;
            __addi.rval = __off;
            __addi.dest = __def;
            __buf.push_back(__addi.data());
            __buf.push_back(" # Passing function argument.\n");
            auto &__dat = __map.at(__in);
            if (!--__dat.out) __work_list.push(__in);
        }

        /* Resolve lazy use and lazy load. */
        load_symbol __sym {load_symbol::FULL, 0, 0};
        for(auto [__def,__addr] : lazy_assign) {
            if (__addr.type == value_type::GLOBAL) {
                __sym.dest = __def;
                __sym.var  = __addr.global.var;
                __buf.push_back(__sym.data());
                __buf.push_back(" # Passing function argument.\n");
            } else { /* Pointer case. */
                __addi.lval = __addr.pointer.reg;
                __addi.rval = __addr.pointer.offset;
                __addi.dest = __def;
                __buf.push_back(__addi.data());
                __buf.push_back(" # Passing function argument.\n");
            }
        }

        load_memory __load {memory_base::WORD, 0, value_type {nullptr}};
        for(auto [__def,__addr] : lazy_load) {
            __load.dest = __def;
            __load.addr = __addr;
            __buf.push_back(__load.data());
            __buf.push_back(" # Passing function argument.\n");
        }
        lazy_assign.clear(); lazy_load.clear();
    }();

    /* Now, only a0 ~ a7 cycles remains. Using t0 as middle temporary. */
    std::array <bool,8> visited = {false};

    arith_immediat __arith {arith_base::ADD, 0, 0, 0};
    auto *__tmp = get_physical(physical_register::t0); // t0 as temporary

    /* Resolve cycles. */
    for(const auto [__reg,__dat] : __map) {
        if (visited.at(__reg->index - __reg->a0)) continue;
        __buf.push_back("    # Swapping part.\n");
        __arith.lval = __reg;
        __arith.rval = 0; // no offset.
        __arith.dest = __tmp;
        __buf.push_back(__arith.data());
        __buf.push_back(" # Swapping function argument.\n");

        auto *__cur = __reg;
        do {
            visited.at(__cur->index - __reg->a0) = true;
            auto &&__ref = __map.at(__cur);
            /* __reg is now stored into __tmp. */
            __arith.lval = __ref.in == __reg ? __tmp : __ref.in;
            __arith.rval = __ref.off;
            __arith.dest = __cur;
            /* Ending of the cycle. */
            __buf.push_back(__arith.data());
            __buf.push_back(" # Swapping function argument.\n");
            __cur = __ref.in;
        } while(__cur != __reg);
    }

    return string_join_array(__buf.begin(),__buf.end());
}


void function::print_entry(std::ostream &__os) const {
    struct node_data {
        physical_register *in {nullptr};
        ssize_t           out    {0};
    };

    /* Mapping from physical register to its inner data. */
    std::unordered_map <physical_register *, node_data> __map;

    move_register __move {0,0};
    store_memory __store {memory_base::WORD, 0, stack_address {}};
    size_t __top = 0;
    for(auto __val : dummy_def) {
        /* Filter out the dead arguments. */
        while (func_ptr->args[__top]->is_dead()) ++__top;
        auto *__use = get_physical(10 + (__top++));
        if (__val.type == value_type::POINTER) {
            auto __def = safe_cast <physical_register *> (__val.pointer.reg);
            /* No operation. */
            if (__def == __use) { __os << "    # Merged!\n"; continue; }
            /* Constantly evaluated case: the source will not be changed. */
            if (__def->index < __use->a0 || __def->index > __use->a7) {
                __move.from = __use;
                __move.dest = __def;
                __os << __move.data() << '\n';
            } else {
                __map[__use].out++;
                __map[__def].in  = __use;
            }
        } else { /* Stack address. */
            __store.from = __use;
            __store.addr = __val;
            __os << __store.data() << '\n';
        }
    }

    std::queue <physical_register *> __work_list;
    for(auto &[__reg,__dat] : __map) if (!__dat.out) __work_list.push(__reg);

    while(!__work_list.empty()) {
        auto *__def = __work_list.front(); __work_list.pop();
        auto __iter = __map.find(__def);
        auto *__in  = __iter->second.in;
        __map.erase(__iter);
        if (__in == nullptr) continue;

        __move.from = __in;
        __move.dest = __def;
        __os << __move.data() << '\n';
        auto &__dat = __map.at(__in);
        if (!--__dat.out) __work_list.push(__in);
    }

    std::array <bool,8> visited = {false};
    auto *__tmp = get_physical(physical_register::t1); // t1 as temporary

    /* Resolve cycles. */
    for(const auto [__reg,__dat] : __map) {
        if (visited.at(__reg->index - __reg->a0)) continue;
        __move.from = __reg;
        __move.dest = __tmp;
        __os << __move.data() << '\n';
        auto *__cur = __reg;
        do {
            visited.at(__cur->index - __reg->a0) = true;
            auto &&__ref = __map.at(__cur);
            /* __reg is now stored into __tmp. */
            __move.from = __ref.in == __reg ? __tmp : __ref.in;
            __move.dest = __cur;
            __os << __move.data() << '\n';
            /* Ending of the cycle. */
            __cur = __ref.in;
        } while(__cur != __reg);
    }
}



void function::print(std::ostream &__os) const {
    __os << __indent << ".globl " << name << '\n';
    __os << name << ":\n";
    __os << "# " << name << ".prework:\n";

    if (save_ra)
        __os << "    sw ra, -4(sp) # Storing return address.\n";

    size_t n = save_ra;
    /* Save those callee save registers. */
    for(auto __tmp : callee_save)
        __os << __indent << "sw "
             << __tmp->data() << ", -" << (++n) * 4 << "(sp)\n";

    /* Move the stack pointer. */
    n = get_stack_space();
    if (n) __os << __indent << "addi sp, sp, -" << n << "\n\n";

    /* This marks the ending of prework. */
    __os << '.' << name << ".prework.end:\n";

    /* TODO: deal with the dummy defs in the function. */
    __os << "# Loading register arguments (a0 ~ a7)...\n";

    /* Load the arguments. */
    print_entry(__os);

    __os << "# Complete loading arguments.\n";
    for(auto __block : blocks) __os << __block->data() << '\n';
    __os << '\n';
}


/* Things to do before returning. */
std::string function::return_data() const {
    std::vector <std::string> __buf;
    __buf.reserve(callee_save.size() + 2);

    /* Restore the stack pointer. */
    size_t n = get_stack_space();
    if (n) __buf.push_back(string_join("    addi sp, sp, ", std::to_string(n),'\n'));

    /* Save those callee save registers. */
    if (save_ra)
        __buf.push_back("    lw ra, -4(sp) # Loading return address.\n");

    n = save_ra;
    for(auto __tmp : callee_save) {
        __buf.push_back(string_join("    lw ",
            __tmp->data(), ", -", std::to_string((++n) * 4), "(sp)\n"));
    }

    return string_join_array(__buf.begin(),__buf.end());
}


std::string call_function::data() const {
    std::vector <std::string> __buf;
    if (op == TAIL) {
        __buf.reserve(3);
        __buf.push_back(function_node::resolve_arg());
        if (self == func) {
            /* No calling convention worry. Simplified to a jump. */
            __buf.push_back(string_join(
                __indent,"j .", func->name,".prework.end"
            ));
        } else {
            __buf.push_back(self->return_data());
            __buf.push_back(string_join(__indent, "tail ", func->name));
        }
    } else {
        __buf.reserve(5);
        __buf.push_back(function_node::resolve_spill());
        __buf.push_back(function_node::resolve_arg());
        __buf.push_back(string_join(__indent, "call ", func->name,'\n'));

        if (is_dest_spilled) {
            store_memory __store {
                memory_base::WORD, 0,
                stack_address {self, spilled_offset}
            };
            __buf.push_back(__store.data() + '\n');
        } else if (dest && dest != get_physical(10) && dest != get_physical(0)) {
            __buf.push_back(string_join(__indent, "mv ", dest->data(), ", a0\n"));
        }

        /* Reload the spilled after the function. */
        __buf.push_back(function_node::resolve_load());
    }

    return string_join_array(__buf.begin(),__buf.end());
}


std::string call_builtin::data() const {
    /* Tailing a builtin function is almost the same as calling it. */


    return "# Not implemented!";
}



}