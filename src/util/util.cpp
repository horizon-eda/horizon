#include "util.hpp"
#include <fstream>
#include <giomm.h>
#include <glibmm/miscutils.h>
#include <glib/gfileutils.h>
#include <glib/gstdio.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef G_OS_WIN32
#include <windows.h>
#include <iostream>
#endif
#include <iomanip>
#include <nlohmann/json.hpp>
#include "alphanum/alphanum.hpp"
#include <gdk/gdkkeysyms.h>
#include "imp/in_tool_action.hpp"
#include "str_util.hpp"
#include "placement.hpp"

#ifdef __OpenBSD__
char *argv0;
#endif

namespace horizon {

std::ifstream make_ifstream(const std::string &filename_utf8, std::ios_base::openmode mode)
{
#ifdef G_OS_WIN32
    auto wfilename = reinterpret_cast<wchar_t *>(g_utf8_to_utf16(filename_utf8.c_str(), -1, NULL, NULL, NULL));
    std::ifstream ifs(wfilename, mode);
    g_free(wfilename);
#else
    std::ifstream ifs(filename_utf8, mode);
#endif
    return ifs;
}

std::ofstream make_ofstream(const std::string &filename_utf8, std::ios_base::openmode mode)
{
#ifdef G_OS_WIN32
    auto wfilename = reinterpret_cast<wchar_t *>(g_utf8_to_utf16(filename_utf8.c_str(), -1, NULL, NULL, NULL));
    std::ofstream ofs(wfilename, mode);
    g_free(wfilename);
#else
    std::ofstream ofs(filename_utf8, mode);
#endif
    return ofs;
}

void save_json_to_file(const std::string &filename, const json &j)
{
    auto ofs = make_ofstream(filename);
    if (!ofs.is_open()) {
        throw std::runtime_error("can't save json " + filename);
        return;
    }
    ofs << std::setw(4) << j;
    ofs.close();
}

json load_json_from_file(const std::string &filename)
{
    json j;
    auto ifs = make_ifstream(filename);
    if (!ifs.is_open()) {
        throw std::runtime_error("file " + filename + " not opened");
    }
    ifs >> j;
    ifs.close();
    return j;
}

json json_from_resource(const std::string &rsrc)
{
    auto json_bytes = Gio::Resource::lookup_data_global(rsrc);
    gsize size = json_bytes->get_size();
    return json::parse((const char *)json_bytes->get_data(size));
}

#ifdef G_OS_WIN32
std::string get_exe_dir()
{
    TCHAR szPath[MAX_PATH];
    if (!GetModuleFileName(NULL, szPath, MAX_PATH)) {
        throw std::runtime_error("can't find executable");
        return "";
    }
    else {
        return Glib::path_get_dirname(szPath);
    }
}

#else
#if defined(__linux__)
std::string get_exe_dir()
{

    char buf[PATH_MAX];
    ssize_t len;
    if ((len = readlink("/proc/self/exe", buf, sizeof(buf) - 1)) != -1) {
        buf[len] = '\0';
        return Glib::path_get_dirname(buf);
    }
    else {
        throw std::runtime_error("can't find executable");
        return "";
    }
}
#elif defined(__FreeBSD__)
#include <sys/sysctl.h>
std::string get_exe_dir()
{
    char buf[PATH_MAX];
    size_t len = sizeof(buf);
    int mib[] = {CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1};
    int ret;
    ret = sysctl(mib, 4, buf, &len, NULL, 0);
    if (ret == 0) {
        return Glib::path_get_dirname(buf);
    }
    else {
        throw std::runtime_error("can't find executable");
        return "";
    }
}
#elif defined(__OpenBSD__)
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
std::string get_exe_dir()
{
    char *path, *p, *p2, buf[PATH_MAX];
    struct stat sb;
    if (strchr(argv0, '/')) {
        if (realpath(argv0, buf)) {
            return Glib::path_get_dirname(buf);
        }
    }
    else {
        if (!(path = getenv("PATH"))) {
            path = strdup("/usr/bin:/bin:/usr/sbin:/sbin:/usr/X11R6/bin:/usr/local/bin:/usr/local/sbin");
        }
        else {
            path = strdup(path);
        }
        for (p = path; (p2 = strsep(&p, ":")) != NULL;) {
            if (*p2 == '\0') {
                getcwd(buf, sizeof(buf));
            }
            else {
                strlcpy(buf, p2, sizeof(buf));
            }
            strlcat(buf, "/", sizeof(buf));
            strlcat(buf, argv0, sizeof(buf));
            if (!stat(buf, &sb) && (sb.st_mode & S_IXUSR)) {
                free(path);
                return Glib::path_get_dirname(buf);
            }
        }
        free(path);
    }
    throw std::runtime_error("can't find executable");
    return "";
}
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
std::string get_exe_dir()
{
    char buf[PATH_MAX];
    uint32_t size = sizeof(buf);
    if (_NSGetExecutablePath(buf, &size) == 0) {
        return Glib::path_get_dirname(buf);
    }
    else {
        throw std::runtime_error("can't find executable");
        return "";
    }
}
#else
#error Dont know how to find path to executable on your OS.
#endif
#endif


void allow_set_foreground_window(int pid)
{
#ifdef G_OS_WIN32
    AllowSetForegroundWindow(pid);
#endif
}

bool endswith(const std::string &haystack, const std::string &needle)
{
    auto pos = haystack.rfind(needle);
    if (pos == std::string::npos)
        return false;
    else
        return (haystack.size() - haystack.rfind(needle)) == needle.size();
}

int strcmp_natural(const std::string &a, const std::string &b)
{
    return doj::alphanum_comp(a, b);
}

int strcmp_natural(const char *a, const char *b)
{
    return doj::alphanum_comp(a, b);
}

std::string get_config_dir()
{
    return Glib::build_filename(Glib::get_user_config_dir(), "horizon");
}

void create_cache_and_config_dir()
{
    auto cache_dir = Glib::get_user_cache_dir();
    g_mkdir_with_parents(cache_dir.c_str(), S_IRWXU);
    auto config_dir = get_config_dir();
    g_mkdir_with_parents(config_dir.c_str(), S_IRWXU);
}

void replace_backslash(std::string &path)
{
#ifdef G_OS_WIN32
    std::replace(path.begin(), path.end(), '\\', '/');
#endif
}

bool compare_files(const std::string &filename_a, const std::string &filename_b)
{
    auto mapped_a = g_mapped_file_new(filename_a.c_str(), false, NULL);
    if (!mapped_a) {
        return false;
    }
    auto mapped_b = g_mapped_file_new(filename_b.c_str(), false, NULL);
    if (!mapped_b) {
        g_mapped_file_unref(mapped_a);
        return false;
    }
    if (g_mapped_file_get_length(mapped_a) != g_mapped_file_get_length(mapped_b)) {
        g_mapped_file_unref(mapped_a);
        g_mapped_file_unref(mapped_b);
        return false;
    }
    auto size = g_mapped_file_get_length(mapped_a);
    auto r = memcmp(g_mapped_file_get_contents(mapped_a), g_mapped_file_get_contents(mapped_b), size);
    g_mapped_file_unref(mapped_a);
    g_mapped_file_unref(mapped_b);
    return r == 0;
}

void find_files_recursive(const std::string &base_path, std::function<void(const std::string &)> cb,
                          const std::string &path)
{
    auto this_path = Glib::build_filename(base_path, path);
    Glib::Dir dir(this_path);
    for (const auto &it : dir) {
        auto itempath = Glib::build_filename(this_path, it);
        if (Glib::file_test(itempath, Glib::FILE_TEST_IS_DIR)) {
            find_files_recursive(base_path, cb, Glib::build_filename(path, it));
        }
        else if (Glib::file_test(itempath, Glib::FILE_TEST_IS_REGULAR)) {
            cb(Glib::build_filename(path, it));
        }
    }
}

json color_to_json(const Color &c)
{
    json j;
    j["r"] = c.r;
    j["g"] = c.g;
    j["b"] = c.b;
    return j;
}


Color color_from_json(const json &j)
{
    Color c;
    c.r = j.at("r").get<float>();
    c.g = j.at("g").get<float>();
    c.b = j.at("b").get<float>();
    return c;
}

json colori_to_json(const ColorI &c)
{
    json j;
    j["r"] = c.r;
    j["g"] = c.g;
    j["b"] = c.b;
    return j;
}


ColorI colori_from_json(const json &j)
{
    ColorI c;
    c.r = j.at("r").get<int>();
    c.g = j.at("g").get<int>();
    c.b = j.at("b").get<int>();
    return c;
}

static std::locale the_locale = std::locale::classic();

void setup_locale()
{
    std::locale::global(std::locale::classic());
    char decimal_sep = '.';
#ifdef G_OS_WIN32
    TCHAR szSep[8];
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, szSep, 8);
    if (szSep[0] == ',') {
        decimal_sep = ',';
    }
#else
    try {
        // Try setting the "native locale", in order to retain
        // things like decimal separator.  This might fail in
        // case the user has no notion of a native locale.
        auto user_locale = std::locale("");
        decimal_sep = std::use_facet<std::numpunct<char>>(user_locale).decimal_point();
    }
    catch (...) {
        // just proceed
    }
#endif

    class comma : public std::numpunct<char> {
    public:
        comma(char c) : dp(c)
        {
        }
        char do_decimal_point() const override
        {
            return dp;
        } // comma

    private:
        const char dp;
    };

    the_locale = std::locale(std::locale::classic(), new comma(decimal_sep));
}

const std::locale &get_locale()
{
    return the_locale;
}

std::string format_m_of_n(unsigned int m, unsigned int n)
{
    auto n_str = std::to_string(n);
    auto digits_max = n_str.size();
    auto m_str = std::to_string(m);
    std::string prefix;
    for (size_t i = 0; i < (digits_max - (int)m_str.size()); i++) {
        prefix += " ";
    }
    return prefix + m_str + "/" + n_str;
}

std::string format_digits(unsigned int m, unsigned int digits_max)
{
    auto m_str = std::to_string(m);
    std::string prefix;
    if (m_str.size() < digits_max) {
        for (size_t i = 0; i < (digits_max - (int)m_str.size()); i++) {
            prefix += " ";
        }
    }
    return prefix + m_str;
}

double parse_si(const std::string &inps)
{
    static const auto regex = Glib::Regex::create(
            "([+-]?)(?:(?:(\\d+)[\\.,]?(\\d*))|(?:[\\.,](\\d+)))(?:[eE]([-+]?)([0-9]+)|\\s*([a-zA-Zµ])|)");
    Glib::ustring inp(inps);
    Glib::MatchInfo ma;
    if (regex->match(inp, ma)) {
        auto sign = ma.fetch(1);
        auto intv = ma.fetch(2);
        auto fracv = ma.fetch(3);
        auto fracv2 = ma.fetch(4);
        auto exp_sign = ma.fetch(5);
        auto exp = ma.fetch(6);
        auto prefix = ma.fetch(7);

        double v = 0;
        if (intv.size()) {
            v = std::stoi(intv);
            if (fracv.size()) {
                v += std::stoi(fracv) * std::pow(10, (int)fracv.size() * -1);
            }
        }
        else {
            v = std::stoi(fracv2) * std::pow(10, (int)fracv2.size() * -1);
        }

        if (exp.size()) {
            int iexp = std::stoi(exp);
            if (exp_sign == "-") {
                iexp *= -1;
            }
            v *= std::pow(10, iexp);
        }
        else if (prefix.size()) {
            int prefix_exp = 0;
            if (prefix == "p")
                prefix_exp = -12;
            else if (prefix == "n" || prefix == "N")
                prefix_exp = -9;
            else if (prefix == "u" || prefix == "U" || prefix == "µ")
                prefix_exp = -6;
            else if (prefix == "m")
                prefix_exp = -3;
            else if (prefix == "k" || prefix == "K")
                prefix_exp = 3;
            else if (prefix == "M")
                prefix_exp = 6;
            else if (prefix == "G" || prefix == "g")
                prefix_exp = 9;
            else if (prefix == "T" || prefix == "t")
                prefix_exp = 12;
            v *= std::pow(10, prefix_exp);
        }
        if (sign == "-")
            v *= -1;

        return v;
    }

    return NAN;
}

void rmdir_recursive(const std::string &dir_name)
{
    Glib::Dir dir(dir_name);
    std::list<std::string> entries(dir.begin(), dir.end());
    for (const auto &it : entries) {
        auto filename = Glib::build_filename(dir_name, it);
        if (Glib::file_test(filename, Glib::FILE_TEST_IS_DIR)) {
            rmdir_recursive(filename);
        }
        else {
            if (g_unlink(filename.c_str()) != 0)
                throw std::runtime_error("g_unlink");
        }
    }
    if (g_rmdir(dir_name.c_str()) != 0)
        throw std::runtime_error("g_rmdir");
}

static bool isvar(char c)
{
    return std::isalnum(c) || c == '_' || c == ':';
}

std::string interpolate_text(const std::string &str,
                             std::function<std::optional<std::string>(const std::string &s)> interpolator)
{

    enum class State { NORMAL, DOLLAR, VAR, VAR_BRACED };
    State state = State::NORMAL;
    std::string out;
    std::string var_current;
    size_t i = 0;
    while (true) {
        const char c = str.c_str()[i++];
        switch (state) {
        case State::NORMAL:
            if (c == '$') {
                state = State::DOLLAR;
            }
            else if (c) {
                out.append(1, c);
            }
            break;

        case State::DOLLAR:
            if (isvar(c)) {
                var_current.clear();
                var_current.append(1, c);
                state = State::VAR;
            }
            else if (c == '{') {
                var_current.clear();
                state = State::VAR_BRACED;
            }
            else {
                out.append("$");
                if (c)
                    out.append(1, c);
                state = State::NORMAL;
            }
            break;

        case State::VAR:
            if (isvar(c)) {
                var_current.append(1, c);
            }
            else {
                if (auto subst = interpolator(var_current)) {
                    out.append(*subst);
                }
                else {
                    out.append("$" + var_current);
                }

                if (c == '$') {
                    state = State::DOLLAR;
                }
                else if (c) {
                    out.append(1, c);
                    state = State::NORMAL;
                }
            }
            break;

        case State::VAR_BRACED:
            if (c == '}') {
                if (auto subst = interpolator(var_current)) {
                    out.append(*subst);
                }
                else {
                    out.append("${" + var_current + "}");
                }
                state = State::NORMAL;
            }
            else {
                var_current.append(1, c);
            }
            break;
        }

        if (!c)
            break;
    }

    return out;
}

std::pair<Coordi, bool> dir_from_action(InToolActionID a)
{
    using I = InToolActionID;
    switch (a) {
    case I::MOVE_UP_FINE:
        return {{0, 1}, true};
    case I::MOVE_UP:
        return {{0, 1}, false};

    case I::MOVE_DOWN_FINE:
        return {{0, -1}, true};
    case I::MOVE_DOWN:
        return {{0, -1}, false};

    case I::MOVE_LEFT_FINE:
        return {{-1, 0}, true};
    case I::MOVE_LEFT:
        return {{-1, 0}, false};

    case I::MOVE_RIGHT_FINE:
        return {{1, 0}, true};
    case I::MOVE_RIGHT:
        return {{1, 0}, false};

    default:
        return {{0, 0}, false};
    }
}

void check_object_type(const json &j, ObjectType type)
{
    if (j.at("type") != object_type_lut.lookup_reverse(type)) {
        throw std::runtime_error("wrong object type");
    }
}

void ensure_parent_dir(const std::string &path)
{
    auto parent = Glib::path_get_dirname(path);
    if (!Glib::file_test(parent, Glib::FILE_TEST_IS_DIR)) {
        Gio::File::create_for_path(parent)->make_directory_with_parents();
    }
}

std::string get_existing_path(const std::string &p)
{
    Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(p);
    while (!file->query_exists()) {
        file = file->get_parent();
    }
    return file->get_path();
}

std::string append_dot_json(const std::string &s)
{
    std::string r = s;
    trim(r);
    if (!endswith(r, ".json")) {
        return r + ".json";
    }
    return r;
}

Orientation get_pin_orientation_for_placement(Orientation pin_orientation, const class Placement &placement)
{
    static const std::map<Orientation, Orientation> omap_90 = {
            {Orientation::LEFT, Orientation::DOWN},
            {Orientation::UP, Orientation::LEFT},
            {Orientation::RIGHT, Orientation::UP},
            {Orientation::DOWN, Orientation::RIGHT},
    };
    static const std::map<Orientation, Orientation> omap_180 = {
            {Orientation::LEFT, Orientation::RIGHT},
            {Orientation::UP, Orientation::DOWN},
            {Orientation::RIGHT, Orientation::LEFT},
            {Orientation::DOWN, Orientation::UP},
    };
    static const std::map<Orientation, Orientation> omap_270 = {
            {Orientation::LEFT, Orientation::UP},
            {Orientation::UP, Orientation::RIGHT},
            {Orientation::RIGHT, Orientation::DOWN},
            {Orientation::DOWN, Orientation::LEFT},
    };
    static const std::map<Orientation, Orientation> omap_mirror = {
            {Orientation::LEFT, Orientation::RIGHT},
            {Orientation::UP, Orientation::UP},
            {Orientation::RIGHT, Orientation::LEFT},
            {Orientation::DOWN, Orientation::DOWN},
    };

    const auto angle = placement.get_angle();
    if (angle == 16384) {
        pin_orientation = omap_90.at(pin_orientation);
    }
    if (angle == 32768) {
        pin_orientation = omap_180.at(pin_orientation);
    }
    if (angle == 49152) {
        pin_orientation = omap_270.at(pin_orientation);
    }
    if (placement.mirror) {
        pin_orientation = omap_mirror.at(pin_orientation);
    }
    return pin_orientation;
}

} // namespace horizon
