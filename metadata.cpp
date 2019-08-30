//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define __STDC_FORMAT_MACROS 1
#include "metadata.h"
#include "messages.h"
#include <regex>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cinttypes>
#include <cassert>

static const std::string cstrlit(gsl::cstring_span text);
static const std::string mangle(gsl::cstring_span name);

static bool is_decint_string(gsl::cstring_span str)
{
    if (!str.empty() && str[0] == '-')
        str = str.subspan(1);

    if (str.empty())
        return false;

    for (char c : str) {
        if (c < '0' || c > '9')
            return false;
    }

    return true;
}

static int extract_widget(pugi::xml_node node, bool is_active, Metadata &md);

int extract_metadata(const pugi::xml_document &doc, Metadata &md, const std::string *mdsource)
{
    pugi::xml_node root = doc.child("faust");

    md.name = root.child_value("name");
    md.author = root.child_value("author");
    md.copyright = root.child_value("copyright");
    md.license = root.child_value("license");
    md.version = root.child_value("version");
    md.classname = root.child_value("classname");
    md.inputs = std::stoi(root.child_value("inputs"));
    md.outputs = std::stoi(root.child_value("outputs"));

    for (pugi::xml_node meta : root.children("meta")) {
        std::string key = meta.attribute("key").value();
        std::string value = meta.child_value();
        md.metadata.emplace_back(key, value);
    }

    for (pugi::xml_node node : root.child("ui").child("activewidgets").children("widget")) {
        if (extract_widget(node, true, md) == -1)
            return -1;
    }

    for (pugi::xml_node node : root.child("ui").child("passivewidgets").children("widget")) {
        if (extract_widget(node, false, md) == -1)
            return -1;
    }

    if (mdsource) {
        std::string line;
        line.reserve(1024);

        bool is_in_class = false;

        std::string &ccode = md.class_code;

        ccode.clear();
        ccode.reserve(8192);

        // add our warning suppressions
        ccode.append(
            "#if defined(__GNUC__)" "\n"
            "#   pragma GCC diagnostic push" "\n"
            "#   pragma GCC diagnostic ignored \"-Wunused-parameter\"" "\n"
            "#endif" "\n"
            "\n");

        // add our overrides for source keywords
        ccode.append(
            "#ifndef FAUSTPP_PRIVATE" "\n"
            "#   define FAUSTPP_PRIVATE private" "\n"
            "#endif" "\n"
            "#ifndef FAUSTPP_PROTECTED" "\n"
            "#   define FAUSTPP_PROTECTED protected" "\n"
            "#endif" "\n"
            "#ifndef FAUSTPP_VIRTUAL" "\n"
            "#   define FAUSTPP_VIRTUAL virtual" "\n"
            "#endif" "\n"
            "\n");

        // add our namespace definitions
        ccode.append(
            "#ifndef FAUSTPP_BEGIN_NAMESPACE" "\n"
            "#   define FAUSTPP_BEGIN_NAMESPACE" "\n"
            "#endif" "\n"
            "#ifndef FAUSTPP_END_NAMESPACE" "\n"
            "#   define FAUSTPP_END_NAMESPACE" "\n"
            "#endif" "\n"
            "\n");

        // open the namespace
        ccode.append(
            "FAUSTPP_BEGIN_NAMESPACE" "\n"
            "\n");
        bool is_in_namespace = true;

        static const std::regex re_include(
            "^\\s*#\\s*include\\s+(.*)$");

        std::istringstream in(*mdsource);
        while (std::getline(in, line)) {
            if (is_in_class) {
                if (line == "<<<<EndFaustClass>>>>")
                    is_in_class = false;
                else {
                    // make sure not to enclose #include in the namespace
                    bool is_include = std::regex_match(line, re_include);
                    if (is_include && is_in_namespace) {
                        ccode.append("FAUSTPP_END_NAMESPACE" "\n");
                        is_in_namespace = false;
                    }
                    else if (!is_include && !is_in_namespace) {
                        ccode.append("FAUSTPP_BEGIN_NAMESPACE" "\n");
                        is_in_namespace = true;
                    }

                    ccode.append(line);
                    ccode.push_back('\n');
                }
            }
            else if (line == "<<<<BeginFaustClass>>>>")
                is_in_class = true;
        }

        // close the namespace
        if (is_in_namespace)
            ccode.append(
                "FAUSTPP_END_NAMESPACE" "\n"
                "\n");

        // add our warning suppressions
        ccode.append(
            "\n"
            "#if defined(__GNUC__)" "\n"
            "#   pragma GCC diagnostic pop" "\n"
            "#endif" "\n");
    }

    return 0;
}

static int extract_widget(pugi::xml_node node, bool is_active, Metadata &md)
{
    Metadata::Widget w;
    w.typestring = node.attribute("type").value();
    w.type = Metadata::Widget::type_from_name(w.typestring);
    if (w.type == (Metadata::Widget::Type)-1)
        return -1;

    w.id = std::stoi(node.attribute("id").value());
    w.label = node.child_value("label");
    w.var = node.child_value("varname");

    // w.symbol = mangle(w.label);

    if (is_active && (w.type == Metadata::Widget::Type::HSlider ||
                      w.type == Metadata::Widget::Type::VSlider ||
                      w.type == Metadata::Widget::Type::NEntry)) {
        w.init = std::stof(node.child_value("init"));
        w.min = std::stof(node.child_value("min"));
        w.max = std::stof(node.child_value("max"));
        w.step = std::stof(node.child_value("step"));
    }
    else if (is_active && (w.type == Metadata::Widget::Type::Button ||
                           w.type == Metadata::Widget::Type::CheckBox)) {
        w.init = 0;
        w.min = 0;
        w.max = 1;
        w.step = 1;
    }
    else if (!is_active && (w.type == Metadata::Widget::Type::VBarGraph ||
                            w.type == Metadata::Widget::Type::HBarGraph)) {
        w.min = std::stof(node.child_value("min"));
        w.max = std::stof(node.child_value("max"));
    }
    else
        return -1;

    for (pugi::xml_node meta : node.children("meta")) {
        std::string key = meta.attribute("key").value();
        std::string value = meta.child_value();
        if (is_decint_string(key) && value.empty())
            continue;
        w.metadata.emplace_back(key, value);

        if (key == "unit")
            w.unit = value;
        else if (key == "scale") {
            w.scalestring = value;
            w.scale = Metadata::Widget::scale_from_name(value);
            if (w.scale == (Metadata::Widget::Scale)-1) {
                warns() << "Unrecognized scale type `" << value << "`\n";
                w.scale = Metadata::Widget::Scale::Linear;
            }
        }
        else if (key == "tooltip")
            w.tooltip = value;
        // else if (key == "md.symbol")
        //     w.symbol = mangle(value);
    }

    (is_active ? md.active : md.passive).push_back(std::move(w));
    return 0;
}

static const std::string cstrlit(gsl::cstring_span text)
{
    std::string lit;
    if (false) {
        lit.push_back('u');
        lit.push_back('8');
    }
    lit.push_back('"');

    for (char c : text) {
        switch (c) {
        case '\a': lit.push_back('\\'); lit.push_back('a'); break;
        case '\b': lit.push_back('\\'); lit.push_back('b'); break;
        case '\t': lit.push_back('\\'); lit.push_back('t'); break;
        case '\n': lit.push_back('\\'); lit.push_back('n'); break;
        case '\v': lit.push_back('\\'); lit.push_back('v'); break;
        case '\f': lit.push_back('\\'); lit.push_back('f'); break;
        case '\r': lit.push_back('\\'); lit.push_back('r'); break;
        case '"': case '\\': lit.push_back('\\'); lit.push_back(c); break;
        default: lit.push_back(c); break;
        }
    }

    lit.push_back('"');
    return lit;
}

static const std::string mangle(gsl::cstring_span name)
{
    std::string id;
    size_t n = name.size();
    id.reserve(n);

    for (size_t i = 0; i < n; ++i) {
        char c = name[i];
        bool al = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
        bool num = c >= '0' && c <= '9';
        c = (!al && (!num || i == 0)) ? '_' : c;
        id.push_back(c);
    }

    return id;
}

Metadata::Widget::Type Metadata::Widget::type_from_name(gsl::cstring_span name)
{
    Metadata::Widget::Type t = (Metadata::Widget::Type)-1;

    if (name == "button")
        t = Type::Button;
    else if (name == "checkbox")
        t = Type::CheckBox;
    else if (name == "vslider")
        t = Type::VSlider;
    else if (name == "hslider")
        t = Type::HSlider;
    else if (name == "nentry")
        t = Type::NEntry;
    else if (name == "vbargraph")
        t = Type::VBarGraph;
    else if (name == "hbargraph")
        t = Type::HBarGraph;

    return t;
}

Metadata::Widget::Scale Metadata::Widget::scale_from_name(gsl::cstring_span name)
{
    Metadata::Widget::Scale s = (Metadata::Widget::Scale)-1;

    if (name == "log")
        s = Scale::Log;
    else if (name == "exp")
        s = Scale::Exp;

    return s;
}

std::ostream &operator<<(std::ostream &o, Metadata::Widget::Type t)
{
    switch (t) {
    case Metadata::Widget::Type::Button: return o << "button";
    case Metadata::Widget::Type::CheckBox: return o << "checkbox";
    case Metadata::Widget::Type::VSlider: return o << "vslider";
    case Metadata::Widget::Type::HSlider: return o << "hslider";
    case Metadata::Widget::Type::NEntry: return o << "nentry";
    case Metadata::Widget::Type::VBarGraph: return o << "vbargraph";
    case Metadata::Widget::Type::HBarGraph: return o << "hbargraph";
    }

    assert(false);
    return o;
}

std::ostream &operator<<(std::ostream &o, Metadata::Widget::Scale s)
{
    switch (s) {
    case Metadata::Widget::Scale::Linear: return o << "linear";
    case Metadata::Widget::Scale::Log: return o << "log";
    case Metadata::Widget::Scale::Exp: return o << "exp";
    }

    assert(false);
    return o;
}

//------------------------------------------------------------------------------
#include "inja/inja.hpp"

void render_metadata(std::ostream &out, const Metadata &md, const std::string &tmplfile, const std::map<std::string, std::string> &defines)
{
    inja::Environment env;
    inja::Template tmpl = env.parse_template(tmplfile);

    if (tmpl.content.empty()) {
        errs() << "The template file is empty or unreadable.\n";
        return;
    }

    nlohmann::json root_obj;

    root_obj["class_code"] = md.class_code;

    root_obj["name"] = md.name;
    root_obj["author"] = md.author;
    root_obj["copyright"] = md.copyright;
    root_obj["license"] = md.license;
    root_obj["version"] = md.version;
    root_obj["class_name"] = md.classname;
    root_obj["file_name"] = md.filename;
    root_obj["inputs"] = md.inputs;
    root_obj["outputs"] = md.outputs;

    nlohmann::json &file_meta = root_obj["meta"];
    for (const auto &meta : md.metadata)
        file_meta[meta.first] = meta.second;

    for (unsigned wtype = 0; wtype < 2; ++wtype) {
        const std::vector<Metadata::Widget> *widget_list = nullptr;
        nlohmann::json *widget_list_obj;

        if (wtype == 0) {
            widget_list = &md.active;
            widget_list_obj = &root_obj["active"];
        }
        else if (wtype == 1) {
            widget_list = &md.passive;
            widget_list_obj = &root_obj["passive"];
        }

        assert(widget_list);

        for (size_t i = 0, n = widget_list->size(); i < n; ++i) {
            const Metadata::Widget &widget = (*widget_list)[i];
            nlohmann::json &widget_obj = widget_list_obj->emplace_back();

            widget_obj["type"] = widget.typestring;
            widget_obj["id"] = widget.id;
            widget_obj["label"] = widget.label;
            widget_obj["var"] = widget.var;
            widget_obj["init"] = widget.init;
            widget_obj["min"] = widget.min;
            widget_obj["max"] = widget.max;
            widget_obj["step"] = widget.step;
            widget_obj["unit"] = widget.unit;
            widget_obj["scale"] = widget.scalestring;
            widget_obj["tooltip"] = widget.tooltip;

            nlohmann::json &widget_meta = widget_obj["meta"];
            for (const auto &meta : widget.metadata)
                widget_meta[meta.first] = meta.second;
        }
    }

    env.add_callback(
        "cstr", 1,
        [](inja::Arguments &args) -> std::string {
            return cstrlit(args.at(0)->get<std::string>());
        });
    env.add_callback(
        "cid", 1,
        [](inja::Arguments &args) -> std::string {
            return mangle(args.at(0)->get<std::string>());
        });

    for (auto &def : defines) {
        unsigned n;
        int64_t i;
        uint64_t u;
        double f;
        if (sscanf(def.second.c_str(), "%" SCNi64 "%n", &i, &n) == 1 && n == def.second.size())
            root_obj[def.first] = i;
        if (sscanf(def.second.c_str(), "%" SCNu64 "%n", &u, &n) == 1 && n == def.second.size())
            root_obj[def.first] = u;
        else if (sscanf(def.second.c_str(), "%lf%n", &f, &n) == 1 && n == def.second.size())
            root_obj[def.first] = f;
        else
            root_obj[def.first] = def.second;
    }

    env.render_to(out, tmpl, root_obj);
}
