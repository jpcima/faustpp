//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "metadata.h"
#include "call_faust.h"
#include "messages.h"
#include "pugixml.hpp"
#include "gsl/gsl-lite.hpp"
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>

#if defined(_WIN32)
    #include <windows.h>
#else
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <unistd.h>
#endif

static const char *g_argv0;

struct Cmd_Args {
    std::string tmplfile;
    std::string dspfile;
    std::string processname;
    std::map<std::string, std::string> defines;
    Faust_Args faustargs;
};

static void display_usage();
static int do_cmdline(Cmd_Args &cmd, int argc, char *argv[]);
static std::string find_template_file(const std::string &name);
static std::string get_tempfile_location(const std::string &name);

int main(int argc, char *argv[])
{
    g_argv0 = argv[0];

    Cmd_Args cmd;
    if (do_cmdline(cmd, argc, argv) == -1) {
        display_usage();
        return 1;
    }

    if (cmd.tmplfile.empty()) {
        errs() << "No architecture file has been specified.\n";
        return 1;
    }

    cmd.tmplfile = find_template_file(cmd.tmplfile);
    if (cmd.tmplfile.empty()) {
        errs() << "Cannot find the architecture file.\n";
        return 1;
    }

    for (const std::string &arg : cmd.faustargs.compile_args) {
        if (arg == "-pn" || arg == "--process-name")
            warns() << "Avoid the flag `-X" << arg << "`, use `-pn` instead.\n";
    }

    Faust_Version faustver;
    if (get_faust_version(faustver) == -1)
        warns() << "Cannot determine the version of Faust.\n";
    else {
        Faust_Version faustminver{0, 9, 85};
        if (faustver < faustminver) {
            errs() << "The Faust version " << faustver << " is too old, the requirement is " << faustminver << ".\n";
            return 1;
        }
    }

    std::string mdfile = get_tempfile_location("md.cpp");
    if (mdfile.empty())
        return 1;

    std::ofstream mds(mdfile);
    if (!mds) {
        errs() << "Could not open the temporary file for writing.\n";
        return 1;
    }
    auto md_cleanup = gsl::finally([&]() { unlink(mdfile.c_str()); });

    {
        gsl::cstring_span text =
            "<<<<BeginFaustClass>>>>\n"
            "<<includeIntrinsic>>\n"
            "<<includeclass>>\n"
            "<<<<EndFaustClass>>>>\n";
        mds.write(text.data(), text.size());
        mds.flush();
        if (!mds) {
            errs() << "Could not write the temporary file.\n";
            return 1;
        }
        mds.close();
    }

    Faust_Args mdargs = cmd.faustargs;
    mdargs.compile_args.push_back("-a");
    mdargs.compile_args.push_back(mdfile);

    pugi::xml_document doc;
    std::string mdsource;
    if (call_faust(cmd.dspfile, cmd.processname, &doc, &mdsource, mdargs) == -1) {
        errs() << "The faust command has failed.\n";
        return 1;
    }

    if (false)
        doc.save(std::cerr);

    Metadata md;
    if (extract_metadata(cmd.dspfile, cmd.processname, doc, md, &mdsource) == -1) {
        errs() << "Could not extract the faust metadata.\n";
        return 1;
    }

#if !defined(_WIN32)
    size_t dspfilesep = cmd.dspfile.rfind('/');
#else
    size_t dspfilesep = cmd.dspfile.find_last_of("\\/");
#endif
    std::string basedspfile;
    if (dspfilesep == std::string::npos)
        md.filename = cmd.dspfile;
    else
        md.filename = cmd.dspfile.substr(dspfilesep + 1);

    render_metadata(std::cout, md, cmd.tmplfile, cmd.defines);
    std::cout.flush();

    return 0;
}

static void display_usage()
{
    std::cerr <<
        "Usage: faustpp [-a <file>] [-D<name=value>] [-X<arg>] <file.dsp>\n"
        "    -a <file>          Use the specified architecture file.\n"
        "    -pn <name>         Use the specified function as the entry point.\n"
        "    -D<name=value>     Pass a definition to the code generator.\n"
        "    -X<arg>            Pass a raw argument to the faust compiler.\n";
}

static int do_cmdline(Cmd_Args &cmd, int argc, char *argv[])
{
    bool moreflags = true;
    unsigned extraindex = 0;

    for (int i = 1; i < argc; ++i) {
        gsl::cstring_span arg = argv[i];

        if (moreflags && arg == "--")
            moreflags = false;
        else if (moreflags && arg == "-a") {
            if (++i == argc) {
                errs() << "The flag `-a` requires an argument.\n";
                return -1;
            }
            cmd.tmplfile = argv[i];
        }
        else if (moreflags && arg == "-pn") {
            if (++i == argc) {
                errs() << "The flag `-pn` requires an argument.\n";
                return -1;
            }
            cmd.processname = argv[i];
        }
        else if (moreflags && arg.subspan(0, 2) == "-D") {
            gsl::cstring_span expr = arg.subspan(2);
            auto pos = std::find(expr.begin(), expr.end(), '=');
            if (pos == expr.begin() || pos == expr.end()) {
                errs() << "The definition is malformed.\n";
                return -1;
            }
            std::string key(expr.begin(), pos);
            std::string value(pos + 1, expr.end());
            cmd.defines[key] = value;
        }
        else if (moreflags && arg.subspan(0, 2) == "-X") {
            cmd.faustargs.compile_args.emplace_back(argv[i] + 2);
        }
        else if (moreflags && !arg.empty() && arg[0] == '-') {
            errs() << "Unrecognized flag `" << arg << "`\n";
            return -1;
        }
        else {
            switch (extraindex) {
            case 0:
                cmd.dspfile = gsl::to_string(arg);
                break;
            default:
                errs() << "Unrecognized positional argument `" << arg << "`\n";
                return -1;
            }
            ++extraindex;
        }
    }

    if (extraindex != 1) {
        errs() << "There must be exactly one positional argument.\n";
        return -1;
    }

    return 0;
}

static std::string find_template_file(const std::string &name)
{
    if (access(name.c_str(), F_OK) == 0)
        return name;

    // second candidate: look in the installed architectures directory

#ifndef _WIN32
    auto is_path_separator = [](char c) { return c == '/'; };
#else
    auto is_path_separator = [](char c) { return c == '/' || c == '\\'; };
#endif

    if (std::find_if(name.begin(), name.end(), is_path_separator) != name.end())
        return std::string{};

    std::string base_dir = g_argv0;
    while (!base_dir.empty() && !is_path_separator(base_dir.back()))
        base_dir.pop_back();
    while (!base_dir.empty() && is_path_separator(base_dir.back()))
        base_dir.pop_back();

    std::string name2;
    if (base_dir.size() >= 3 && !memcmp(&base_dir[base_dir.size() - 3], "bin", 3) &&
        (base_dir.size() == 3 || is_path_separator(base_dir[base_dir.size() - 4])))
    {
        name2.append(base_dir.data(), base_dir.size() - 3);
        name2.append("share/faustpp/architectures/");
        name2.append(name);
    }

    if (name2.empty() || access(name2.c_str(), F_OK) != 0)
        return std::string{};

    return name2;
}

static std::string get_tempfile_location(const std::string &name)
{
#if defined(_WIN32)
#   error Not implemented...
#endif

    std::string homedir;
    if (const char *env = getenv("HOME"))
        homedir.assign(env);

    if (homedir.empty()) {
        errs() << "Could not determine the home directory.\n";
        return std::string();
    }

#if defined(__APPLE__)
    std::string tempdir = homedir + "/Library";
    mkdir(tempdir.c_str(), 0755);
    tempdir += "/Caches";
    mkdir(tempdir.c_str(), 0755);
    tempdir += "/TemporaryItems";
    mkdir(tempdir.c_str(), 0755);
    tempdir += "/faustpp";
    mkdir(tempdir.c_str(), 0755);
    return tempdir + "/" + std::to_string(getpid()) + "-" + name;
#else
    std::string tempdir = homedir + "/.cache";
    mkdir(tempdir.c_str(), 0755);
    tempdir += "/faustpp";
    mkdir(tempdir.c_str(), 0755);
    return tempdir + "/" + std::to_string(getpid()) + "-" + name;
#endif
}
