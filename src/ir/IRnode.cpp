#include "IRnode.h"


namespace dark::IR {


std::string block_stmt::data() const {
    std::vector <std::string> __tmp;
    __tmp.reserve(stmt.size() + 2);
    __tmp.push_back(label + ":\n");
    for(auto __p : stmt) __tmp.push_back("    " + __p->data());
    __tmp.push_back("\n");
    return string_join_array(__tmp.begin(),__tmp.end());
}


std::vector <phi_stmt *> block_stmt::get_phi_block() const {
    std::vector <phi_stmt *> __vec;
    for(auto __node : stmt) {
        if(auto *__phi = dynamic_cast <phi_stmt *> (__node))
            __vec.push_back(__phi);
        else break;
    } return __vec;
}

bool block_stmt::is_unreachable() const {
    return dynamic_cast <unreachable_stmt *> (stmt.front());
}


std::string function::data() const {
    std::string __arg; /* Arglist. */
    bool __first = true;
    for(auto *__p : args) {
        if (!__first) __arg += ','; 
        else          __first = false;
        __arg += __p->get_value_type().name();
        __arg += ' ';
        __arg += __p->data();
    }

    std::vector <std::string> __tmp;

    __tmp.reserve(stmt.size() + 2);
    __tmp.push_back(string_join(
        "define ",type.name()," @",name,
        "(",std::move(__arg),") {\n"
    ));
    for(auto __p : stmt) __tmp.push_back(__p->data());
    __tmp.push_back(string_join("}\n\n"));

    return string_join_array(__tmp.begin(),__tmp.end());
};

std::string function::declare() const {
    std::string __arg; /* Arglist. */
    bool __first = true;
    for(auto *__p : args) {
        if(!__first) __arg += ','; 
        else        __first = false;
        __arg += __p->get_value_type().name();
    }

    return string_join(
        "declare ",type.name()," @",name,
        "(",std::move(__arg),")\n"
    );
}


std::string compare_stmt::data() const {
    runtime_assert("Invalid compare expression!",
        lvar->get_value_type() == rvar->get_value_type(),
        dest->get_value_type().name() == "i1"
    );
    return string_join(
        dest->data()," = icmp "
        ,str[op],' ',lvar->get_value_type().name(),' '
        ,lvar->data(),", ",rvar->data(),'\n'
    );
}


std::string binary_stmt::data() const {
    runtime_assert("Invalid binary expression!",
        lvar->get_value_type() ==
            rvar->get_value_type(),
        lvar->get_value_type() ==
            dest->get_value_type()
    );
    return string_join(
        dest->data()," = ",str[op],' ',
        dest->get_value_type().name(),' '
        ,lvar->data(),", ",rvar->data(),'\n'
    );
};


std::string branch_stmt::data() const {
    runtime_assert("Branch condition must be boolean type!",
        cond->get_value_type().name() == "i1"
    );
    return string_join(
        "br i1 ",cond->data(),
        ", label %",br[0]->label,
        ", label %",br[1]->label,
        '\n'
    );
}


std::string call_stmt::data() const {
    std::string __arg; /* Arglist. */
    bool __first = true;
    for(auto *__p : args) {
        if (!__first) __arg += ','; 
        else          __first = false;
        __arg += __p->get_value_type().name();
        __arg += ' ';
        __arg += __p->data();
    }
    std::string __prefix;
    if(func->type.name() != "void")
        __prefix = dest->data() + " = "; 
    return string_join(
        __prefix, "call ",
        func->type.name()," @",func->name,
        "(",std::move(__arg),")\n"
    );
}


std::string load_stmt::data() const {
    runtime_assert("Invalid load statement",
        src->get_point_type() == dst->get_value_type()
    );
    return string_join(
        dst->data(), " = load ",
        dst->get_value_type().name(),", ptr ",src->data(),'\n'
    );
}


std::string store_stmt::data() const {
    runtime_assert("Invalid load statement!",
        src->get_value_type() == dst->get_point_type()
    );
    return string_join(
        "store ",src->get_value_type().name(),' ',
        src->data(),", ptr ",dst->data(),'\n'
    );
}


std::string return_stmt::data() const {
    runtime_assert("Invalid return statement!",
        !rval || func->type == rval->get_value_type(),
            rval || func->type.name() == "void"
    );
    if(!rval) return "ret void\n";
    else {
        return string_join(
            "ret ",func->type.name(),' ',rval->data(),'\n'
        );
    }
}


std::string allocate_stmt::data() const {
    return string_join(
        dest->data()," = alloca ",dest->get_point_type().name(),'\n'
    );
}


std::string get_stmt::data() const {
    std::string __suffix;
    if(mem != NPOS) __suffix = ", i32 " + std::to_string(mem);
    else runtime_assert("Invalid get statement!",
        idx->get_value_type().name() == "i32",
        dst->get_value_type() == src->get_value_type()
    );
    return string_join(
        dst->data()," = getelementptr ",src->get_point_type().name(),
        ", ptr ",src->data(), ", i32 ",idx->data(),__suffix,'\n'
    );
}

std::string phi_stmt::data() const {
    std::vector <std::string> __tmp;

    __tmp.reserve(cond.size() + 2);
    __tmp.push_back(string_join(
        dest->data()," = phi ", dest->get_value_type().name())
    );
    for(auto [value,label] : cond) {
        runtime_assert("Invalid phi statement!",
            dest->get_value_type() == value->get_value_type()
        );
        __tmp.push_back(string_join(
            (label == cond.front().label ? " [ " : " , [ "),
            value->data()," , %",label->label," ]"
        ));
    } __tmp.push_back("\n");

    return string_join_array(__tmp.begin(),__tmp.end());
}



}