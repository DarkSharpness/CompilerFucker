#pragma once

#include "ASTnode.h"

namespace dark::AST {

/**
 * @brief Structure:
 * Every scope node contains one scope pointer.
 * It points to the father in the scope tree.
 * This will greatly lower the cost of storing
 * all the info in one node.
*/
struct scope {
    /* Pointer to the father scope. */
    scope *prev = nullptr;

    /* Mapping from a name to a variable/function */
    std::map <std::string,AST::identifier *> data;


    AST::identifier *find(const std::string &__name) const noexcept {
        if(this == nullptr) return nullptr;
        auto __iter = data.find(__name);
        /* Find the name recursively...... */
        if(__iter == data.end()) return prev->find(__name);
        else                     return __iter->second;
    }

    /* Return whether the name already exists. */
    bool exist(const std::string &__name) const noexcept
    { return data.find(__name) == data.end(); }

    /* Return whether this insertion is successful. */
    bool insert(const std::string &__name,AST::identifier *__p)
    { return data.insert({__name,__p}).second; } 

};





}
