//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "pugixml/pugixml.hpp"
#include "gsl/gsl-lite.hpp"
#include <string>
#include <vector>
#include <map>
#include <iosfwd>

struct Metadata {
    std::string name;
    std::string author;
    std::string copyright;
    std::string license;
    std::string version;
    std::string classname;
    std::string filename;
    std::vector<std::pair<std::string, std::string>> metadata;
    unsigned inputs = 0;
    unsigned outputs = 0;

    struct Widget {
        enum class Type { Button, CheckBox, VSlider, HSlider, NEntry, VBarGraph, HBarGraph };
        enum class Scale { Linear, Log, Exp };

        Type type = (Type)-1;
        std::string typestring;
        int id = 0;
        std::string label;
        std::string var;
        // std::string symbol;
        float init = 0;
        float min = 0;
        float max = 0;
        float step = 0;
        std::vector<std::pair<std::string, std::string>> metadata;

        // metadata interpretation
        std::string unit;
        Scale scale = Scale::Linear;
        std::string scalestring;
        std::string tooltip;

        static Type type_from_name(gsl::cstring_span name);
        static Scale scale_from_name(gsl::cstring_span name);
    };

    std::vector<Widget> active;
    std::vector<Widget> passive;

    std::string intrinsic_code;
    std::string class_code;
};

int extract_metadata(const pugi::xml_document &doc, Metadata &md, const std::string *mdsource);
void render_metadata(std::ostream &out, const Metadata &md, const std::string &tmplfile, const std::map<std::string, std::string> &defines);

std::ostream &operator<<(std::ostream &o, Metadata::Widget::Type t);
std::ostream &operator<<(std::ostream &o, Metadata::Widget::Scale s);
