#include "component.hpp"
#include "block.hpp"
#include "logger/logger.hpp"
#include "pool/part.hpp"
#include "nlohmann/json.hpp"
#include "pool/ipool.hpp"
#include "util/pin_direction_accumulator.hpp"

namespace horizon {

json Connection::serialize() const
{
    json j;
    if (net)
        j["net"] = net->uuid;
    else
        j["net"] = nullptr;
    return j;
}

Connection::Connection(const json &j, Block *block)
{
    if (j.at("net").is_null()) {
        net = nullptr;
        return;
    }
    if (block) {
        UUID net_uu = j.at("net").get<std::string>();
        net = block->get_net(net_uu);
        if (net == nullptr) {
            throw std::runtime_error("net " + (std::string)net_uu + " not found");
        }
    }
    else
        net.uuid = j.at("net").get<std::string>();
}


Pin::Direction Component::get_effective_direction(const Component::AltPinInfo &alt, const Pin &pin)
{
    if (alt.use_custom_name)
        return alt.custom_direction;
    PinDirectionAccumulator acc;
    if (alt.use_primary_name)
        acc.accumulate(pin.direction);
    for (const auto &n : alt.pin_names) {
        if (pin.names.count(n))
            acc.accumulate(pin.names.at(n).direction);
    }
    return acc.get().value_or(pin.direction);
}

Pin::Direction Component::get_effective_direction(const UUIDPath<2> &path) const
{
    const auto &pin = entity->gates.at(path.at(0)).unit->pins.at(path.at(1));
    if (alt_pins.count(path))
        return get_effective_direction(alt_pins.at(path), pin);
    else
        return pin.direction;
}


void Component::AltPinInfo::update_for_index(int index, const Pin &pin)
{
    if (index == -2) {
        use_custom_name = true;
    }
    else if (index == -1) {
        use_primary_name = true;
    }
    else {
        const auto uu = Pin::alternate_name_uuid_from_index(index);
        if (pin.names.count(uu))
            pin_names.insert(uu);
    }
}

Component::AltPinInfo::AltPinInfo(const json &j, const Pin &pin)
    : use_primary_name(j.at("use_primary_name").get<bool>()), use_custom_name(j.at("use_custom_name").get<bool>()),
      custom_name(j.at("custom_name").get<std::string>()),
      custom_direction(Pin::direction_lut.lookup(j.at("custom_direction").get<std::string>()))
{
    for (const auto &it : j.at("pin_names")) {
        const UUID u{it.get<std::string>()};
        if (pin.names.count(u))
            pin_names.insert(u);
    }
}

json Component::AltPinInfo::serialize() const
{
    json j;
    j["use_primary_name"] = use_primary_name;
    j["use_custom_name"] = use_custom_name;
    j["custom_name"] = custom_name;
    j["custom_direction"] = Pin::direction_lut.lookup_reverse(custom_direction);
    j["pin_names"] = json::array();
    for (const auto &it : pin_names)
        j["pin_names"].push_back((std::string)it);
    return j;
}


Component::Component(const UUID &uu, const json &j, IPool &pool, Block *block)
    : uuid(uu), entity(pool.get_entity(j.at("entity").get<std::string>())), refdes(j.at("refdes").get<std::string>()),
      value(j.at("value").get<std::string>()), nopopulate(j.value("nopopulate", false))
{
    if (j.count("part")) {
        part = pool.get_part(j.at("part").get<std::string>());
        if (part->entity->uuid != entity->uuid)
            part = nullptr;
    }
    if (j.count("group"))
        group = UUID(j.at("group").get<std::string>());
    if (j.count("tag"))
        tag = UUID(j.at("tag").get<std::string>());

    {
        const json &o = j["connections"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            UUIDPath<2> u(it.key());
            if (entity->gates.count(u.at(0)) > 0) {
                const auto &g = entity->gates.at(u.at(0));
                if (g.unit->pins.count(u.at(1)) > 0) {
                    try {
                        connections.emplace(std::make_pair(u, Connection(it.value(), block)));
                    }
                    catch (const std::exception &e) {
                        Logger::log_critical("error loading connection to " + refdes + "." + g.name + "."
                                                     + g.unit->pins.at(u.at(1)).primary_name,
                                             Logger::Domain::BLOCK, e.what());
                    }
                }
                else {
                    Logger::log_critical("connection to nonexistent pin at " + refdes + "." + g.name,
                                         Logger::Domain::BLOCK);
                }
            }
            else {
                Logger::log_critical("connection to nonexistent gate at " + refdes, Logger::Domain::BLOCK);
            }
        }
    }

    if (j.count("alt_pins")) { // new one
        for (const auto &[key, val] : j.at("alt_pins").items()) {
            UUIDPath<2> u(key);
            if (entity->gates.count(u.at(0))) {
                const auto &g = entity->gates.at(u.at(0));
                if (g.unit->pins.count(u.at(1))) {
                    const auto &p = g.unit->pins.at(u.at(1));
                    alt_pins.emplace(std::piecewise_construct, std::forward_as_tuple(u), std::forward_as_tuple(val, p));
                }
            }
        }
    }
    else if (j.count("pin_names")) { // old one
        for (const auto &[key, val] : j.at("pin_names").items()) {
            UUIDPath<2> u(key);
            if (entity->gates.count(u.at(0))) {
                const auto &g = entity->gates.at(u.at(0));
                if (g.unit->pins.count(u.at(1))) {
                    const auto &p = g.unit->pins.at(u.at(1));
                    auto &info = alt_pins[u];
                    std::set<UUID> s;
                    if (val.is_number_integer()) {
                        const auto index = val.get<int>();
                        info.update_for_index(index, p);
                    }
                    else if (val.is_array()) {
                        for (const auto &x : val) {
                            info.update_for_index(x.get<int>(), p);
                        }
                    }
                }
            }
        }
    }

    if (j.count("custom_pin_names")) {
        const json &o = j["custom_pin_names"];
        for (const auto &[key, val] : o.items()) {
            UUIDPath<2> u(key);
            if (entity->gates.count(u.at(0)) > 0) {
                const auto &g = entity->gates.at(u.at(0));
                if (g.unit->pins.count(u.at(1)) > 0) {
                    alt_pins[u].custom_name = val.get<std::string>();
                }
            }
        }
    }
    if (j.count("href")) {
        href = uuid_vec_from_string(j.at("href").get<std::string>());
    }
}
Component::Component(const UUID &uu) : uuid(uu)
{
}
UUID Component::get_uuid() const
{
    return uuid;
}
std::string Component::replace_text(const std::string &t, bool *replaced) const
{
    if (replaced)
        *replaced = false;
    if (t == "$REFDES" || t == "$RD") {
        if (replaced)
            *replaced = true;
        return refdes;
    }
    else if (t == "$VALUE") {
        if (replaced)
            *replaced = true;
        if (part)
            return part->get_value();
        else
            return value;
    }
    else if (t == "$MPN") {
        if (part) {
            if (replaced)
                *replaced = true;
            return part->get_MPN();
        }
        return t;
    }
    else {
        return t;
    }
}

const std::string &Component::get_prefix() const
{
    if (part)
        return part->get_prefix();
    else
        return entity->prefix;
}

ItemSet Component::get_pool_items_used() const
{
    ItemSet items_needed;

    items_needed.emplace(ObjectType::ENTITY, entity->uuid);
    for (const auto &it_gate : entity->gates) {
        items_needed.emplace(ObjectType::UNIT, it_gate.second.unit->uuid);
    }
    if (part) {
        const auto part_items = part->get_pool_items_used();
        items_needed.insert(part_items.begin(), part_items.end());
    }

    return items_needed;
}

json Component::serialize() const
{
    json j;
    j["refdes"] = refdes;
    j["value"] = value;
    j["entity"] = (std::string)entity->uuid;
    j["group"] = (std::string)group;
    j["tag"] = (std::string)tag;
    j["connections"] = json::object();
    if (nopopulate) {
        j["nopopulate"] = true;
    }
    if (part != nullptr) {
        j["part"] = part->uuid;
    }
    for (const auto &it : connections) {
        j["connections"][(std::string)it.first] = it.second.serialize();
    }
    j["pin_names"] = json::object(); // for compatibility
    j["alt_pins"] = json::object();  // for compatibility
    for (const auto &[k, v] : alt_pins) {
        j["alt_pins"][(std::string)k] = v.serialize();
    }
    if (href.size())
        j["href"] = uuid_vec_to_string(href);
    return j;
}
} // namespace horizon
