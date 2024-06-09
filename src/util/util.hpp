#pragma once
#include "common/common.hpp"
#include "nlohmann/json_fwd.hpp"
#include <string>
#include <vector>
#include <functional>
#include <locale>
#include <fstream>
#include <optional>

namespace horizon {
using json = nlohmann::json;
enum class InToolActionID;

std::ifstream make_ifstream(const std::string &filename_utf8, std::ios_base::openmode mode = std::ios_base::in);
std::ofstream make_ofstream(const std::string &filename_utf8, std::ios_base::openmode mode = std::ios_base::out);

void save_json_to_file(const std::string &filename, const json &j);
json load_json_from_file(const std::string &filename);
std::string get_exe_dir();
void allow_set_foreground_window(int pid);
void setup_locale();
const std::locale &get_locale();

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

template <typename Set, typename F> static void set_erase_if(Set &m, F pred)
{
    for (auto it = m.begin(), last = m.end(); it != last;) {
        if (pred(*it)) {
            it = m.erase(it);
        }
        else {
            ++it;
        }
    }
}

bool endswith(const std::string &haystack, const std::string &needle);

int strcmp_natural(const std::string &a, const std::string &b);
int strcmp_natural(const char *a, const char *b);
void create_cache_and_config_dir();
std::string get_config_dir();

void replace_backslash(std::string &path);
json json_from_resource(const std::string &rsrc);
bool compare_files(const std::string &filename_a, const std::string &filename_b);
void find_files_recursive(const std::string &base_path, std::function<void(const std::string &)> cb,
                          const std::string &path = "");

Color color_from_json(const json &j);
json color_to_json(const Color &c);

ColorI colori_from_json(const json &j);
json colori_to_json(const ColorI &c);

std::string format_m_of_n(unsigned int m, unsigned int n);
std::string format_digits(unsigned int m, unsigned int digits_max);
double parse_si(const std::string &inps);

void rmdir_recursive(const std::string &dir_name);
std::string interpolate_text(const std::string &str,
                             std::function<std::optional<std::string>(const std::string &s)> interpolator);

std::pair<Coordi, bool> dir_from_action(InToolActionID a);


template <typename T> constexpr bool any_of(T value, std::initializer_list<T> choices)
{
    return std::count(choices.begin(), choices.end(), value);
}

void check_object_type(const json &j, ObjectType type);
void ensure_parent_dir(const std::string &path);
std::string get_existing_path(const std::string &p);

std::string append_dot_json(const std::string &s);

Orientation get_pin_orientation_for_placement(Orientation pin_orientation, const class Placement &placement);

} // namespace horizon
