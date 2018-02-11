#pragma once
#include "nlohmann/json_fwd.hpp"
#include <glibmm/datetime.h>
#include <map>

namespace horizon {
using json = nlohmann::json;
void recent_from_json(std::map<std::string, Glib::DateTime> &recent_items, const json &j);
std::vector<std::pair<std::string, Glib::DateTime>>
recent_sort(const std::map<std::string, Glib::DateTime> &recent_items);
} // namespace horizon
