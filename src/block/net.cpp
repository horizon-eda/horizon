#include "net.hpp"
#include "block.hpp"
#include "common/lut.hpp"
#include <nlohmann/json.hpp>


namespace horizon {

static const LutEnumStr<Net::PowerSymbolStyle> power_symbol_style_lut = {
        {"gnd", Net::PowerSymbolStyle::GND},
        {"earth", Net::PowerSymbolStyle::EARTH},
        {"dot", Net::PowerSymbolStyle::DOT},
        {"antenna", Net::PowerSymbolStyle::ANTENNA},
};

Net::Net(const UUID &uu, const json &j, Block &block) : Net(uu, j)
{
    net_class = &block.net_classes.at(j.at("net_class").get<std::string>());
}
Net::Net(const UUID &uu, const json &j)
    : uuid(uu), name(j.at("name").get<std::string>()), is_power(j.value("is_power", false)),
      power_symbol_name_visible(j.value("power_symbol_name_visible", true)), is_port(j.value("is_port", false))
{
    net_class.uuid = j.at("net_class").get<std::string>();
    if (j.count("diffpair")) {
        UUID diffpair_uuid(j.at("diffpair").get<std::string>());
        if (diffpair_uuid) {
            diffpair.uuid = diffpair_uuid;
            diffpair_primary = true;
        }
    }
    if (j.count("power_symbol_style"))
        power_symbol_style = power_symbol_style_lut.lookup(j.at("power_symbol_style"));
    if (j.count("port_direction"))
        port_direction = Pin::direction_lut.lookup(j.at("port_direction"));
    if (j.count("hrefs")) {
        for (const auto &it : j.at("hrefs")) {
            hrefs.push_back(uuid_vec_from_string(it.get<std::string>()));
        }
    }
}

Net::Net(const UUID &uu) : uuid(uu) {};

UUID Net::get_uuid() const
{
    return uuid;
}

json Net::serialize() const
{
    json j;
    j["name"] = name;
    j["is_power"] = is_power;
    j["net_class"] = net_class->uuid;
    j["power_symbol_name_visible"] = power_symbol_name_visible;
    j["power_symbol_style"] = power_symbol_style_lut.lookup_reverse(power_symbol_style);
    if (diffpair_primary && diffpair)
        j["diffpair"] = diffpair->uuid;
    j["is_port"] = is_port;
    j["port_direction"] = Pin::direction_lut.lookup_reverse(port_direction);
    if (hrefs.size()) {
        auto a = json::array();
        for (const auto &it : hrefs)
            a.push_back(uuid_vec_to_string(it));
        j["hrefs"] = a;
    }
    return j;
}

bool Net::is_named() const
{
    return name.size() > 0;
}
} // namespace horizon
