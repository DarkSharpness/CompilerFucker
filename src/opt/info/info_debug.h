#include "collector.h"



namespace dark::OPT {


/* This function is used to print the information of the function. */
void print_info(const function_info &__info,std::ostream &__os = std::cerr) {
    __os << "----------------------\n";
    __os << "Caller: " << __info.func->name << "\nCallee:";
    for (auto __callee : __info.real_info->recursive_func)
        __os << " " << __callee->name;

    __os << "\nArgument state:\n";
    auto __iter = __info.func->args.begin();
    for (auto __arg : __info.func->args) {
        __os << "    " << __arg->data() << " : " << 
            (uint32_t) __arg->leak_flag << " " << (uint32_t) __arg->used_flag << '\n';
    }

    __os << "Global variable:\n ";
    for(auto [__var,__state] : __info.used_global_var) {
        __os << "    " << __var->data() << " : ";
        switch(__state) {
            case function_info::LOAD : __os << "Load only!"; break;
            case function_info::STORE: __os << "Store only!"; break;
            case function_info::BOTH : __os << "Load & Store!"; break;
            default: __os << "??";
        } __os << '\n';
    }

    __os << "Local temporary:\n";
    for(auto &[__var,__use] : __info.use_map) {
        __os << __var->data() << " : ";
        auto __ptr = __use.get_impl_ptr <reliance> ();
        __os <<  (uint32_t) __ptr->leak_flag << " " << (uint32_t) __ptr->used_flag << '\n';
    }

    const char *__msg[4] = { "NONE","IN ONLY","OUT ONLY","IN AND OUT" };
    __os << "Inout state: " << __msg[__info.func->inout_state];
    __os << "\n----------------------\n";
}


}