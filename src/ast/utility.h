#pragma once

#include <iostream>

namespace dark {

struct error {
    error(std::string __s) {
        std::cout << __s << '\n';
        exit(1);
    }
};

}
