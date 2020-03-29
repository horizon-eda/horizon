#include "entity.hpp"
#include "pool.hpp"
#include "nlohmann/json.hpp"
#include "util/util.hpp"

namespace horizon {

Entity::Entity(const UUID &uu, const json &j, Pool &pool)
    : uuid(uu), name(j.at("name").get<std::string>()), manufacturer(j.value("manufacturer", "")),
      prefix(j.at("prefix").get<std::string>())

{
    {
        const json &o = j["gates"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            gates.emplace(std::make_pair(u, Gate(u, it.value(), pool)));
        }
    }
    if (j.count("tags")) {
        tags = j.at("tags").get<std::set<std::string>>();
    }
}

Entity::Entity(const UUID &uu) : uuid(uu)
{
}

Entity Entity::new_from_file(const std::string &filename, Pool &pool)
{
    auto j = load_json_from_file(filename);
    return Entity(UUID(j.at("uuid").get<std::string>()), j, pool);
}

json Entity::serialize() const
{
    json j;
    j["type"] = "entity";
    j["name"] = name;
    j["manufacturer"] = manufacturer;
    j["uuid"] = (std::string)uuid;
    j["prefix"] = prefix;
    j["tags"] = tags;
    j["gates"] = json::object();
    for (const auto &it : gates) {
        j["gates"][(std::string)it.first] = it.second.serialize();
    }
    return j;
}

void Entity::update_refs(Pool &pool)
{
    for (auto &it : gates) {
        it.second.unit = pool.get_unit(it.second.unit.uuid);
    }
}

UUID Entity::get_uuid() const
{
    return uuid;
}
} // namespace horizon
