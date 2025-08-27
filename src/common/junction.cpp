#include "junction.hpp"
#include "lut.hpp"
#include <nlohmann/json.hpp>

namespace horizon {
using json = nlohmann::json;

Junction::Junction(const UUID &uu, const json &j) : uuid(uu), position(j.at("position").get<std::vector<int64_t>>())
{
}

UUID Junction::get_uuid() const
{
    return uuid;
}

Junction::Junction(const UUID &uu) : uuid(uu)
{
}

void Junction::clear()
{
    connected_lines.clear();
    connected_arcs.clear();
    layer = 10000;
}

bool Junction::only_lines_arcs_connected() const
{
    return true;
}

json Junction::serialize() const
{
    json j;
    j["position"] = position.as_array();
    return j;
}
} // namespace horizon
