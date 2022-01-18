#include "unit.hpp"
#include "nlohmann/json.hpp"
#include "util/util.hpp"

namespace horizon {
const LutEnumStr<Pin::Direction> Pin::direction_lut = {
        {"output", Pin::Direction::OUTPUT},
        {"input", Pin::Direction::INPUT},
        {"bidirectional", Pin::Direction::BIDIRECTIONAL},
        {"open_collector", Pin::Direction::OPEN_COLLECTOR},
        {"power_input", Pin::Direction::POWER_INPUT},
        {"power_output", Pin::Direction::POWER_OUTPUT},
        {"passive", Pin::Direction::PASSIVE},
        {"not_connected", Pin::Direction::NOT_CONNECTED},
};

const std::vector<std::pair<Pin::Direction, std::string>> Pin::direction_names = {
        {Pin::Direction::INPUT, "Input"},
        {Pin::Direction::OUTPUT, "Output"},
        {Pin::Direction::BIDIRECTIONAL, "Bidirectional"},
        {Pin::Direction::PASSIVE, "Passive"},
        {Pin::Direction::POWER_INPUT, "Power Input"},
        {Pin::Direction::POWER_OUTPUT, "Power Output"},
        {Pin::Direction::OPEN_COLLECTOR, "Open Collector"},
        {Pin::Direction::NOT_CONNECTED, "Not Connected"},
};

const std::map<Pin::Direction, std::string> Pin::direction_abbreviations = {
        {Pin::Direction::INPUT, "In"},           {Pin::Direction::OUTPUT, "Out"},
        {Pin::Direction::BIDIRECTIONAL, "BiDi"}, {Pin::Direction::PASSIVE, "Passive"},
        {Pin::Direction::POWER_INPUT, "PIn"},    {Pin::Direction::POWER_OUTPUT, "POut"},
        {Pin::Direction::OPEN_COLLECTOR, "OC"},  {Pin::Direction::NOT_CONNECTED, "NC"},
};

Pin::AlternateName::AlternateName(const std::string &n, Direction dir) : name(n), direction(dir)
{
}

Pin::AlternateName::AlternateName(const json &j)
    : name(j.at("name").get<std::string>()), direction(direction_lut.lookup(j.at("direction").get<std::string>()))
{
}

json Pin::AlternateName::serialize() const
{
    json j;
    j["name"] = name;
    j["direction"] = direction_lut.lookup_reverse(direction);
    return j;
}

UUID Pin::alternate_name_uuid_from_index(int idx)
{
    if (idx > 65535 || idx < 0)
        throw std::domain_error("alt pin name index out of range");
    static const UUID ns{"3d1181ab-a2bf-4ddb-98ff-f91c3a817979"};
    std::array<uint8_t, 2> buf;
    buf.at(0) = idx & 0xff;
    buf.at(1) = (idx >> 8) & 0xff;
    return UUID::UUID5(ns, buf.data(), buf.size());
}

Pin::Pin(const UUID &uu, const json &j)
    : uuid(uu), primary_name(j.at("primary_name").get<std::string>()),
      direction(direction_lut.lookup(j.at("direction"))), swap_group(j.value("swap_group", 0))
{
    if (j.count("alt_names")) {
        const auto &j_names = j.at("alt_names");
        for (const auto &[k, v] : j_names.items()) {
            const auto u = UUID(k);
            names.emplace(u, v);
        }
    }
    else if (j.count("names")) {
        const auto &j_names = j.at("names");
        int index = 0;
        for (const auto &name : j_names) {
            const auto u = alternate_name_uuid_from_index(index);
            names.emplace(std::piecewise_construct, std::forward_as_tuple(u),
                          std::forward_as_tuple(name.get<std::string>(), direction));
            index++;
        }
    }
}

Pin::Pin(const UUID &uu) : uuid(uu)
{
}

json Pin::serialize() const
{
    json j;
    j["primary_name"] = primary_name;
    j["direction"] = direction_lut.lookup_reverse(direction);
    j["swap_group"] = swap_group;
    j["names"] = json::array(); // for compatibility
    if (names.size()) {
        j["alt_names"] = json::object();
        for (const auto &[k, v] : names) {
            j["alt_names"][(std::string)k] = v.serialize();
        }
    }
    return j;
}

UUID Pin::get_uuid() const
{
    return uuid;
}

static const unsigned int app_version = 1;

unsigned int Unit::get_app_version()
{
    return app_version;
}

Unit::Unit(const UUID &uu, const json &j)
    : uuid(uu), name(j.at("name").get<std::string>()), manufacturer(j.value("manufacturer", "")),
      version(app_version, j)
{
    check_object_type(j, ObjectType::UNIT);
    version.check(ObjectType::UNIT, name, uuid);
    const json &o = j.at("pins");
    for (auto it = o.cbegin(); it != o.cend(); ++it) {
        auto pin_uuid = UUID(it.key());
        pins.insert(std::make_pair(pin_uuid, Pin(pin_uuid, it.value())));
    }
}

Unit::Unit(const UUID &uu) : uuid(uu), version(app_version)
{
}

Unit Unit::new_from_file(const std::string &filename)
{
    auto j = load_json_from_file(filename);
    return Unit(UUID(j.at("uuid").get<std::string>()), j);
}

json Unit::serialize() const
{
    json j;
    if (auto v = get_required_version())
        j["version"] = v;
    j["type"] = "unit";
    j["name"] = name;
    j["manufacturer"] = manufacturer;
    j["uuid"] = (std::string)uuid;
    j["pins"] = json::object();
    for (const auto &it : pins) {
        j["pins"][(std::string)it.first] = it.second.serialize();
    }
    return j;
}

unsigned int Unit::get_required_version() const
{
    for (const auto &[uu, pin] : pins) {
        if (pin.names.size())
            return 1;
    }
    return 0;
}

UUID Unit::get_uuid() const
{
    return uuid;
}
} // namespace horizon
