//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "pugixml/pugixml.hpp"
#include <string>
#include <vector>
#include <iosfwd>

struct Faust_Args {
    std::vector<std::string> compile_args;
};

int call_faust(
    const std::string &dspfile,
    pugi::xml_document *docmd,
    std::string *cppsource,
    const Faust_Args &faustargs);

struct Faust_Version {
    Faust_Version() {}
    Faust_Version(unsigned maj, unsigned min, unsigned pat) : number{maj, min, pat} {}
    bool operator<(const Faust_Version &oth) const;
    unsigned number[3] = {};
};

int get_faust_version(Faust_Version &version);

std::ostream &operator<<(std::ostream &os, const Faust_Version &ver);
