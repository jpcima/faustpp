//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define __STDC_FORMAT_MACROS 1
#include "metadata.h"
#include "messages.h"
#include <nonstd/string_view.hpp>
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
static int extract_io(pugi::xml_node root, const std::string &dspfile, const std::string &processname, unsigned index, bool is_output, Metadata &md);

int extract_metadata(const std::string &dspfile, const std::string &processname, const pugi::xml_document &doc, Metadata &md, const std::string *mdsource)
{
    pugi::xml_node root = doc.child("faust");

    md.name = root.child_value("name");
    md.author = root.child_value("author");
    md.copyright = root.child_value("copyright");
    md.license = root.child_value("license");
    md.version = root.child_value("version");
    md.classname = root.child_value("classname");
    md.processname = processname.empty() ? std::string("process") : processname;
    md.inputs.resize(std::stoi(root.child_value("inputs")));
    md.outputs.resize(std::stoi(root.child_value("outputs")));

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

    for (unsigned i = 0, n = md.inputs.size(); i < n; ++i)
        extract_io(root, dspfile, processname, i, false, md);

    for (unsigned i = 0, n = md.outputs.size(); i < n; ++i)
        extract_io(root, dspfile, processname, i, true, md);

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

static void extract_faustlike_controller_string(
    nonstd::string_view value, std::string &name,
    std::vector<std::pair<std::string, std::string>> &valuemap)
{
    name.clear();
    name.reserve(256);
    valuemap.clear();
    valuemap.reserve(16);

    auto trim = [](nonstd::string_view s) -> nonstd::string_view {
        auto is_whitespace = [](char c) -> bool { return c == ' ' && c == '\t'; };
        while (!s.empty() && is_whitespace(s.front())) s.remove_prefix(1);
        while (!s.empty() && is_whitespace(s.back())) s.remove_suffix(1);
        return s;
    };

    while (!value.empty()) {
        size_t pos = value.find('[');
        if (pos == value.npos) {
            name.append(value.begin(), value.end());
            value = nonstd::string_view();
        }
        else {
            name.append(value.begin(), value.begin() + pos);
            value.remove_prefix(pos + 1);

            pos = value.find(']');
            nonstd::string_view mdtext;
            if (pos == value.npos) {
                mdtext = value;
                value = nonstd::string_view();
            }
            else {
                mdtext = value.substr(0, pos);
                value.remove_prefix(pos + 1);
            }

            nonstd::string_view mdkey;
            nonstd::string_view mdvalue;
            pos = mdtext.find(':');
            if (pos == mdtext.npos)
                mdkey = mdtext;
            else {
                mdkey = mdtext.substr(0, pos);
                mdvalue = mdtext.substr(pos + 1);
            }

            valuemap.emplace_back(trim(mdkey), trim(mdvalue));
        }
    }

    name = trim(name).to_string();
}

static int extract_io(pugi::xml_node root, const std::string &dspfile, const std::string &processname, unsigned index, bool is_output, Metadata &md)
{
    unsigned count = (unsigned)(is_output ? md.outputs.size() : md.inputs.size());

    std::string key;
    key.reserve(256);

    key.append(dspfile);
    key.push_back('/');
    key.append(processname);
    key.append(is_output ? ":output" : ":input");
    key.append(std::to_string(index));

    pugi::xml_node meta = root.find_child([&key](pugi::xml_node node) -> bool {
        return gsl::cstring_span("meta") == node.name() &&
            key == node.attribute("key").value();
    });
    if (!meta)
        return -1;

    Metadata::IO &io = is_output ? md.outputs[index] : md.inputs[index];
    extract_faustlike_controller_string(meta.child_value(), io.name, io.metadata);

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

    if (name.empty())
        name = "_";

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
#include "jinja2cpp/template.h"
#include "jinja2cpp/template_env.h"
#include "jinja2cpp/user_callable.h"
#include "jinja2cpp/string_helpers.h"
#include "jinja2cpp/generic_list.h"
#include "jinja2cpp/generic_list_iterator.h"
#include "jinja2cpp/filesystem_handler.h"

static jinja2::ValuesMap make_global_environment(
    const Metadata &md, const std::map<std::string, std::string> &defines);

#ifndef _WIN32
static bool is_path_separator(char c) { return c == '/'; };
#else
static bool is_path_separator(char c) { return c == '/' || c == '\\'; };
#endif

/* exception which only us catch, not deriving std::exception */
class RenderFailure {
public:
    explicit RenderFailure(std::string msg) : msg(std::move(msg)) {}
    const std::string &message() const noexcept { return msg; }
private:
    std::string msg;
};

void render_metadata(std::ostream &out, const Metadata &md, const std::string &tmplfile, const std::map<std::string, std::string> &defines)
{
    std::string tmplbase;
    std::string tmpldir = tmplfile;
    while (!tmpldir.empty() && !is_path_separator(tmpldir.back()))
        tmpldir.pop_back();
    if (!tmpldir.empty())
        tmplbase = tmplfile.substr(tmpldir.size());
    else {
        tmpldir = "./";
        tmplbase = tmplfile;
    }

    ///
    jinja2::TemplateEnv env;
    env.AddFilesystemHandler(
        std::string(), jinja2::FilesystemHandlerPtr(new jinja2::RealFileSystem(tmpldir)));

#if 0
    jinja2::Settings settings = env.GetSettings();
    settings.trimBlocks = true;
    settings.lstripBlocks = true;
    env.SetSettings(settings);
#endif

    auto tmpl = env.LoadTemplate(tmplbase);
    if (!tmpl) {
        errs() << tmpl.error() << '\n';
        return;
    }

    for (std::pair<const std::string, jinja2::Value> &kv : make_global_environment(md, defines))
        env.AddGlobal(kv.first, std::move(kv.second));

    try {
        auto result = tmpl->RenderAsString(jinja2::ValuesMap{});
        if (!result)
            errs() << result.error() << '\n';
        else {
            out.write(result->data(), result->size());
            if (result->empty() || result->back() != '\n')
                out.put('\n');
            out.flush();
        }
    }
    catch (RenderFailure &rf) {
        errs() << "Received error from template: " << rf.message() << '\n';
    }
}

static jinja2::Value parse_value_string(const std::string &value)
{
    unsigned n;
    int64_t i;
    double f;
    if (sscanf(value.c_str(), "%" SCNi64 "%n", &i, &n) == 1 && n == value.size())
        return i;
    else if (sscanf(value.c_str(), "%lf%n", &f, &n) == 1 && n == value.size())
        return f;
    else
        return value;
}

static jinja2::ValuesMap make_global_environment(
    const Metadata &md, const std::map<std::string, std::string> &defines)
{
    jinja2::ValuesMap root_obj;

    root_obj["class_code"] = md.class_code;

    root_obj["name"] = md.name;
    root_obj["author"] = md.author;
    root_obj["copyright"] = md.copyright;
    root_obj["license"] = md.license;
    root_obj["version"] = md.version;
    root_obj["class_name"] = md.classname;
    root_obj["process_name"] = md.processname;
    root_obj["file_name"] = md.filename;
    root_obj["inputs"] = (int64_t)md.inputs.size();
    root_obj["outputs"] = (int64_t)md.outputs.size();

    jinja2::ValuesMap &file_meta = (root_obj["meta"] = jinja2::ValuesMap()).asMap();
    for (const auto &meta : md.metadata)
        file_meta[meta.first] = parse_value_string(meta.second);

    for (unsigned wtype = 0; wtype < 2; ++wtype) {
        const std::vector<Metadata::Widget> *widget_list = nullptr;
        jinja2::ValuesList *widget_list_obj;

        if (wtype == 0) {
            widget_list = &md.active;
            widget_list_obj = &((root_obj["active"] = jinja2::ValuesList()).asList());
        }
        else if (wtype == 1) {
            widget_list = &md.passive;
            widget_list_obj = &((root_obj["passive"] = jinja2::ValuesList()).asList());
        }

        assert(widget_list);

        for (size_t i = 0, n = widget_list->size(); i < n; ++i) {
            const Metadata::Widget &widget = (*widget_list)[i];

            jinja2::ValuesMap &widget_obj = (widget_list_obj->emplace_back(jinja2::ValuesMap()),
                                             widget_list_obj->back()).asMap();

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

            jinja2::ValuesMap &widget_meta = (widget_obj["meta"] = jinja2::ValuesMap()).asMap();
            for (const auto &meta : widget.metadata)
                widget_meta[meta.first] = parse_value_string(meta.second);
        }
    }

    for (unsigned iotype = 0; iotype < 2; ++iotype) {
        const std::vector<Metadata::IO> *io_list = nullptr;
        jinja2::ValuesList *io_list_obj;

        if (iotype == 0) {
            io_list = &md.inputs;
#warning TODO choose a better name
            io_list_obj = &((root_obj["ins"] = jinja2::ValuesList()).asList());
        }
        else if (iotype == 1) {
            io_list = &md.outputs;
#warning TODO choose a better name
            io_list_obj = &((root_obj["outs"] = jinja2::ValuesList()).asList());
        }

        assert(io_list);

        for (size_t i = 0, n = io_list->size(); i < n; ++i) {
            const Metadata::IO &io = (*io_list)[i];

            jinja2::ValuesMap &io_obj = (io_list_obj->emplace_back(jinja2::ValuesMap()),
                                         io_list_obj->back()).asMap();

            io_obj["name"] = io.name;

            jinja2::ValuesMap &io_meta = (io_obj["meta"] = jinja2::ValuesMap()).asMap();
            for (const auto &meta : io.metadata)
                io_meta[meta.first] = parse_value_string(meta.second);
        }
    }

    root_obj["cstr"] = jinja2::MakeCallable(
        [](const nonstd::string_view &x) -> std::string { return cstrlit(x); },
        jinja2::ArgInfo("x", true));

    root_obj["cid"] = jinja2::MakeCallable(
        [](const nonstd::string_view &x) -> std::string { return mangle(x); },
        jinja2::ArgInfo("x", true));

    root_obj["fail"] = jinja2::MakeCallable(
        [](nonstd::string_view x) -> nonstd::string_view
        {
            if (x.empty())
                x = "failure without a message";
            throw RenderFailure(x.to_string());
        },
        jinja2::ArgInfo("x", false, nonstd::string_view()));

    for (auto &def : defines)
        root_obj[def.first] = parse_value_string(def.second);

    return root_obj;
}
