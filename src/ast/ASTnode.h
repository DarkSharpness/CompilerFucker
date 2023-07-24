#pragma once

#include <map>
#include <vector>
#include <iostream>
#include <stddef.h>


namespace dark::AST {

struct node;
struct arugment;
struct object;
struct statement;
struct variable;


/* Node with  */
struct node {
    // size_t pos;
    virtual void print() {
        std::cout << "AST node!" << std::endl;
    }
};

struct type_name {
    ssize_t     info;
    std::string type;

    /* Dimension of the object. */
    size_t dimension() const { return info < 0 ? ~info : info; }

    /* Whether the expression is assignable. */
    bool assignable() const { return !(info < 0); }
};


/* Argument with name only. */
struct argument {
    std::string type; /* Type of the variable. */
    std::string name; /* Name of the function. */
};


/* Variable definition. */
struct variable : node , argument {
    expression *init_list;
    void print() override {
        std::cout
            << "Variable signature:\n"
            << type << ' ' << name;
        if(init_list) {
            std::cout << "Initialization list:";
            init_list->print();
        }
    }
};


/* Function definition. */
struct function : node {
    argument info; /* Function name and return type. */
    std::vector <argument> arg_list; /* Argument list. */
    std::vector <statement *> body;
    void print() override {
        std::cout 
            << "Function signature:\n" 
            << info.type << ' ' << info.name
            << '(';
        for(size_t i = 0 ; i < arg_list.size() ; ++i) {
            if(i != 0) std::cout << ',';
            std::cout << arg_list[i].type << ' '
                      << arg_list[i].name;
        }
        std::cout << ")\nFunction body:\n";
        for(auto iter : body)
            iter->print();
    }
};


/* Mapping from variable name to variable type. */
using variable_scope = std::map <std::string,std::string>;
/* Mapping from function name to specific function. */
using function_scope = std::map <std::string,function>;


/* Class definition. */
struct object : node {
    std::string name; /* Class name.           */
    function    ctor; /* Contructor function. */

    /* Mapping from variable name to type name */
    variable_scope var;
    /* Mapping from variable name to function. */
    function_scope fun;

};



struct statement : node {

};

struct expression : node {

};

struct wrapper {
    variable_scope var;
    function_scope fun;
    node root;
};

}