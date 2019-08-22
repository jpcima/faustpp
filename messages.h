//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <iostream>

struct errs {
    errs() { std::cerr << "\033[1;31m"; }
    ~errs() { std::cerr << "\033[0m"; }
    template <class T> errs &operator<<(const T &x) { std::cerr << x; return *this; }
};

struct warns {
    warns() { std::cerr << "\033[1;33m"; }
    ~warns() { std::cerr << "\033[0m"; }
    template <class T> warns &operator<<(const T &x) { std::cerr << x; return *this; }
};
