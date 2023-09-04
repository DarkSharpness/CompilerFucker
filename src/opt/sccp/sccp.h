#pragma once

#include "optnode.h"


namespace dark::OPT {

/* Tries to work out a constant int constant folding stage. */
IR::literal *get_constant_value(IR::node *,const std::vector <IR::node *> &);


struct constant_propagatior {
    constant_propagatior(IR::function *__func,node *__entry) {}
};

}