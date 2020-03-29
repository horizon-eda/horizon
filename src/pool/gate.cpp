#include "gate.hpp"
#include "pool.hpp"
#include "nlohmann/json.hpp"

namespace horizon {

Gate::Gate(const UUID &uu, const json &j, Pool &pool)
    : uuid(uu), name(j.at("name").get<std::string>()), suffix(j.at("suffix").get<std::string>()),
      swap_group(j.value("swap_group", 0)), unit(pool.get_unit(j.at("unit").get<std::string>()))

{
}

Gate::Gate(const UUID &uu) : uuid(uu)
{
}

UUID Gate::get_uuid() const
{
    return uuid;
}

json Gate::serialize() const
{
    json j;
    j["name"] = name;
    j["suffix"] = suffix;
    j["swap_group"] = swap_group;
    j["unit"] = (std::string)unit->uuid;
    return j;
}

} // namespace horizon
