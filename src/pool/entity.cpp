#include "entity.hpp"
#include "ipool.hpp"
#include "nlohmann/json.hpp"
#include "util/util.hpp"

namespace horizon {

static const unsigned int app_version = 0;

Entity::Entity(const UUID &uu, const json &j, IPool &pool)
    : uuid(uu), name(j.at("name").get<std::string>()), manufacturer(j.value("manufacturer", "")),
      prefix(j.at("prefix").get<std::string>()), version(app_version, j)
{
    check_object_type(j, ObjectType::ENTITY);
    version.check(ObjectType::ENTITY, name, uuid);
    {
        const json &o = j.at("gates");
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            gates.emplace(std::piecewise_construct, std::forward_as_tuple(u),
                          std::forward_as_tuple(u, it.value(), pool));
        }
    }
    if (j.count("tags")) {
        tags = j.at("tags").get<std::set<std::string>>();
    }
}

Entity::Entity(const UUID &uu) : uuid(uu), version(app_version)
{
}

Entity Entity::new_from_file(const std::string &filename, IPool &pool)
{
    auto j = load_json_from_file(filename);
    return Entity(UUID(j.at("uuid").get<std::string>()), j, pool);
}

json Entity::serialize() const
{
    json j;
    version.serialize(j);
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

void Entity::update_refs(IPool &pool)
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
