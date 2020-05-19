//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "call_faust.h"
#include "gsl/gsl-lite.hpp"
#include <sys/stat.h>
#include <sys/wait.h>
#include <spawn.h>
#include <unistd.h>
#include <map>
#include <fstream>
#include <sstream>
#include <random>
#include <regex>

extern "C" {
    extern char **environ;
}

static int execute(char *argv[], int out_fd = -1);
static int mktempdir(char *tmp);

static int apply_workarounds(pugi::xml_document &docmd, std::string &cppsource);

//------------------------------------------------------------------------------
int call_faust(
    const std::string &dspfile,
    pugi::xml_document *docmd,
    std::string *cppsource,
    const Faust_Args &faustargs)
{
    char workdir[] = P_tmpdir "/faustXXXXXX";
    if (mktempdir(workdir) == -1)
        return 1;
    auto workdir_cleanup = gsl::finally([&]() { rmdir(workdir); });

    gsl::cstring_span dspfilebase = dspfile;
    {
        size_t index = dspfile.rfind('/');
        if (index != std::string::npos)
            dspfilebase = dspfilebase.subspan(index + 1);
    }

    const std::string xmlfilebase = gsl::to_string(dspfilebase) + ".xml";
    const std::string cppfilebase = gsl::to_string(dspfilebase) + ".cpp";
    const std::string xmlfile = std::string(workdir) + '/' + xmlfilebase;
    const std::string cppfile = std::string(workdir) + '/' + cppfilebase;

    std::vector<char *> fargv {
        (char *)"faust",
        (char *)"-O",
        (char *)workdir,
        (char *)"-o",
        (char *)cppfilebase.c_str(),
        (char *)dspfile.c_str(),
    };

    if (docmd)
        fargv.push_back((char *)"-xml");

    if (const char *program = getenv("FAUST"))
        fargv[0] = (char *)program;

    for (const std::string &arg : faustargs.compile_args)
        fargv.push_back((char *)arg.c_str());

    fargv.push_back(nullptr);
    if (execute(fargv.data()) == -1)
        return -1;

    auto xml_cleanup =
        gsl::finally([&]() {
                         if (docmd)
                             unlink(xmlfile.c_str());
                         unlink(cppfile.c_str());
                     });

    std::string cppsourcebuf;
    if (!cppsource)
        cppsource = &cppsourcebuf;
    {
        std::ifstream cppin(cppfile, std::ios::ate);
        if (!cppin)
            return 1;
        std::streamsize size = cppin.tellg();
        cppsource->reserve(size);
        cppin.seekg(0);
        cppsource->assign((std::istreambuf_iterator<char>(cppin)), std::istreambuf_iterator<char>());
        if (!cppin)
            return 1;
    }

    if (docmd) {
        pugi::xml_parse_result xmlret = docmd->load_file(xmlfile.c_str());
        if (!xmlret)
            return -1;

        if (apply_workarounds(*docmd, *cppsource) == -1)
            return -1;
    }

    return 0;
}

int get_faust_version(Faust_Version &version)
{
    char *fargv[] = {(char *)"faust", (char *)"--version", nullptr};

    if (const char *program = getenv("FAUST"))
        fargv[0] = (char *)program;

    FILE *tmp = tmpfile();
    if (!tmp)
        return -1;
    auto file_cleanup = gsl::finally([&]() { fclose(tmp); });

    if (execute(fargv, fileno(tmp)) != 0)
        return -1;

    fflush(tmp);
    rewind(tmp);

    std::string line;
    line.reserve(256);

    for (int c; (c = fgetc(tmp)) != EOF && c != '\r' && c != '\n';)
        line.push_back(c);

    if (ferror(tmp))
        return -1;

    std::transform(
        line.begin(), line.end(), line.begin(),
        [](char c) -> char { return (c >= 'A' && c <= 'Z') ? (c - 'A' + 'a') : c; });

    size_t index = line.rfind("version");
    if (index == line.npos)
        return -1;

    const char *cstr = line.c_str() + index + 7;
    while (*cstr == ' ' || *cstr == '\t')
        ++cstr;

    if (sscanf(cstr, "%u.%u.%u", &version.number[0], &version.number[1], &version.number[2]) < 1)
        return -1;

    return 0;
}

static bool parse_cstrlit(std::string &dst, const gsl::cstring_span src)
{
    size_t n = src.size();
    if (src.size() < 2 || src[0] != '"' || src[n - 1] != '"')
        return false;

    for (size_t i = 1; i < n - 1; ++i) {
        char c = src[i];
        if (c != '\\')
            dst.push_back(c);
        else {
            if (++i == n - 1)
                return false;
            c = src[i];
            switch (c) {
            case '0': dst.push_back('\0'); break;
            case 'a': dst.push_back('\a'); break;
            case 'b': dst.push_back('\b'); break;
            case 't': dst.push_back('\t'); break;
            case 'n': dst.push_back('\n'); break;
            case 'v': dst.push_back('\v'); break;
            case 'f': dst.push_back('\f'); break;
            case 'r': dst.push_back('\r'); break;
            default: dst.push_back(c); break;
            }
        }
    }

    return true;
}

static int apply_workarounds(pugi::xml_document &docmd, std::string &cppsource)
{
    pugi::xml_node root = docmd.child("faust");

    bool has_meta = root.find_node(
        [](pugi::xml_node node) -> bool { return node.name() == gsl::cstring_span("meta"); });

    if (!has_meta) {
        // workaround: extract the metadata from source file
        std::istringstream in(cppsource);

        //
        std::map<std::string, pugi::xml_node> widget_nodes;
        for (pugi::xml_node node : root.child("ui").child("activewidgets").children("widget"))
            widget_nodes[node.child_value("varname")] = node;
        for (pugi::xml_node node : root.child("ui").child("passivewidgets").children("widget"))
            widget_nodes[node.child_value("varname")] = node;

        //
        std::string line;
        do {
            line.clear();
            std::getline(in, line);

            #define RE_STRLIT "\"(?:\\\\.|[^\"\\\\])*\""
            #define RE_IDENT  "[a-zA-Z_][0-9a-zA-Z_]*"

            static const std::regex re_global(
                "^\\s*m->declare\\((" RE_STRLIT "), (" RE_STRLIT ")\\);");
            static const std::regex re_control(
                "^\\s*ui_interface->declare\\(&(" RE_IDENT "), "
                "(" RE_STRLIT "), (" RE_STRLIT ")\\);");

            #undef RE_STRLIT
            #undef RE_IDENT

            std::smatch match;

            if (std::regex_match(line, match, re_global)) {
                std::string key;
                std::string value;
                if (parse_cstrlit(key, match[1].str()) &&
                    parse_cstrlit(value, match[2].str()))
                {
                    pugi::xml_node meta = root.append_child("meta");
                    meta.append_attribute("key").set_value(key.c_str());
                    meta.text().set(value.c_str());
                }
            }
            else if (std::regex_match(line, match, re_control)) {
                std::string varname = match[1].str();
                std::string key;
                std::string value;
                if (parse_cstrlit(key, match[2].str()) &&
                    parse_cstrlit(value, match[3].str()))
                {
                    auto it = widget_nodes.find(varname);
                    if (it != widget_nodes.end()) {
                        pugi::xml_node meta = it->second.append_child("meta");
                        meta.append_attribute("key").set_value(key.c_str());
                        meta.text().set(value.c_str());
                    }
                }
            }
        } while (in);

        if (in.bad())
            return -1;
    }

    {
        std::string cppsourcemod;
        std::istringstream in(cppsource);

        // workaround: allow to modify member visibility
        // workaround: allow to bypass the virtual keyword

        static const std::regex re_private(
            "^(\\s*)private(\\s*:.*)$");
        static const std::regex re_protected(
            "^(\\s*)protected(\\s*:.*)$");
        static const std::regex re_virtual(
            "^(\\s*)virtual([ \\t].*$|$)");

        //
        std::string line;
        do {
            line.clear();
            std::getline(in, line);

            std::smatch match;

            if (std::regex_match(line, match, re_private))
                line = match[1].str() + "FAUSTPP_PRIVATE" + match[2].str();
            else if (std::regex_match(line, match, re_protected))
                line = match[1].str() + "FAUSTPP_PROTECTED" + match[2].str();
            else if (std::regex_match(line, match, re_virtual))
                line = match[1].str() + "FAUSTPP_VIRTUAL" + match[2].str();

            cppsourcemod.append(line);
            cppsourcemod.push_back('\n');
        } while (in);

        cppsource = cppsourcemod;
    }

    return 0;
}

//------------------------------------------------------------------------------
static int execute(char *argv[], int out_fd)
{
    posix_spawn_file_actions_t fa;
    if (posix_spawn_file_actions_init(&fa) == -1)
        return -1;
    auto fa_cleanup = gsl::finally([&]() { posix_spawn_file_actions_destroy(&fa); });

    if (out_fd != -1) {
        if (posix_spawn_file_actions_adddup2(&fa, out_fd, STDOUT_FILENO) != 0)
            return -1;
    }

    posix_spawnattr_t sa;
    if (posix_spawnattr_init(&sa) == -1)
        return -1;
    auto sa_cleanup = gsl::finally([&]() { posix_spawnattr_destroy(&sa); });

    pid_t pid;
    if (posix_spawnp(&pid, argv[0], &fa, &sa, argv, environ) == -1)
        return -1;

    int status;
    if (waitpid(pid, &status, 0) == -1)
        return -1;

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        return -1;

    return 0;
}

static int mktempdir(char *tmp)
{
    static std::minstd_rand rnd{std::random_device{}()};
    size_t tmplen = std::char_traits<char>::length(tmp);

    int ret = -1;
    while (ret == -1) {
        const char alphabet[32+1] = "0123456789abcdefghijklmnopqrstuv";
        uint32_t number = rnd();
        for (size_t i = 0; i < 6; ++i)
            tmp[tmplen - 1 - i] = alphabet[(number >> (5 * i)) & 31];
        ret = mkdir(tmp, 0700);
        if (ret == -1 && errno != EEXIST)
            return -1;
    }

    return ret;
}

///
bool Faust_Version::operator<(const Faust_Version &oth) const
{
    for (unsigned i = 0; i < 3; ++i)
        if (number[i] != oth.number[i])
            return number[i] < oth.number[i];
    return false;
}

std::ostream &operator<<(std::ostream &os, const Faust_Version &ver)
{
    return os << ver.number[0] << '.' << ver.number[1] << '.' << ver.number[2];
}
