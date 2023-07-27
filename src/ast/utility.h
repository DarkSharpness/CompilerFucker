#pragma once

#include <iostream>

namespace dark {

struct error {
    error(std::string __s) { std::cerr << __s << '\n'; }
};

}
