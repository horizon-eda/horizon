#pragma once
#include "common/common.hpp"
#include "nlohmann/json_fwd.hpp"
#include <string>
#include <vector>
#include <functional>
#include <locale>
#include <fstream>

namespace horizon {
using json = nlohmann::json;

std::ifstream make_ifstream(const std::string &filename_utf8, std::ios_base::openmode mode = std::ios_base::in);
std::ofstream make_ofstream(const std::string &filename_utf8, std::ios_base::openmode mode = std::ios_base::out);

void save_json_to_file(const std::string &filename, const json &j);
json load_json_from_file(const std::string &filename);
int orientation_to_angle(Orientation o);
std::string get_exe_dir();
void allow_set_foreground_window(int pid);
std::string coord_to_string(const Coordf &c, bool delta = false);
std::string dim_to_string(int64_t x, bool with_sign = true);
std::string angle_to_string(int angle, bool pos_only = true);
void setup_locale();
const std::locale &get_locale();

int64_t round_multiple(int64_t x, int64_t mul);

template <typename T, typename U> std::vector<T> dynamic_cast_vector(const std::vector<U> &cin)
{
    std::vector<T> out;
    out.reserve(cin.size());
    std::transform(cin.begin(), cin.end(), std::back_inserter(out), [](auto x) { return dynamic_cast<T>(x); });
    return out;
}

template <typename Map, typename F> static void map_erase_if(Map &m, F pred)
{
    for (typename Map::iterator i = m.begin(); (i = std::find_if(i, m.end(), pred)) != m.end(); m.erase(i++))
        ;
}

bool endswith(const std::string &haystack, const std::string &needle);

template <typename T> int sgn(T val)
{
    return (T(0) < val) - (val < T(0));
}

int strcmp_natural(const std::string &a, const std::string &b);
int strcmp_natural(const char *a, const char *b);
void create_config_dir();
std::string get_config_dir();

void replace_backslash(std::string &path);
json json_from_resource(const std::string &rsrc);
bool compare_files(const std::string &filename_a, const std::string &filename_b);
void find_files_recursive(const std::string &base_path, std::function<void(const std::string &)> cb,
                          const std::string &path = "");

Color color_from_json(const json &j);
json color_to_json(const Color &c);

std::string format_m_of_n(unsigned int m, unsigned int n);
std::string format_digits(unsigned int m, unsigned int digits_max);
double parse_si(const std::string &inps);

void rmdir_recursive(const std::string &dir_name);
std::string replace_placeholders(const std::string &s, std::function<std::string(const std::string &)> fn,
                                 bool keep_empty);

Coordi dir_from_arrow_key(unsigned int key);

template <typename T> std::pair<Coord<T>, Coord<T>> pad_bbox(std::pair<Coord<T>, Coord<T>> bb, T pad)
{
    bb.first.x -= pad;
    bb.first.y -= pad;

    bb.second.x += pad;
    bb.second.y += pad;
    return bb;
}

} // namespace horizon
