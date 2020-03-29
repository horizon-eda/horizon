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

Pin::Pin(const UUID &uu, const json &j)
    : uuid(uu), primary_name(j.at("primary_name").get<std::string>()),
      direction(direction_lut.lookup(j.at("direction"))), swap_group(j.value("swap_group", 0)),
      names(j.value("names", std::vector<std::string>()))
{
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
    j["names"] = names;
    return j;
}

UUID Pin::get_uuid() const
{
    return uuid;
}

Unit::Unit(const UUID &uu, const json &j)
    : uuid(uu), name(j.at("name").get<std::string>()), manufacturer(j.value("manufacturer", ""))
{
    const json &o = j.at("pins");
    for (auto it = o.cbegin(); it != o.cend(); ++it) {
        auto pin_uuid = UUID(it.key());
        pins.insert(std::make_pair(pin_uuid, Pin(pin_uuid, it.value())));
    }
}

Unit::Unit(const UUID &uu) : uuid(uu)
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

UUID Unit::get_uuid() const
{
    return uuid;
}
} // namespace horizon
