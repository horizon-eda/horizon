#pragma once
#include "nlohmann/json_fwd.hpp"
#include "util/uuid.hpp"
#include "util/uuid_ptr.hpp"
#include "util/uuid_vec.hpp"
#include "pool/unit.hpp"

namespace horizon {
using json = nlohmann::json;

class Net {
public:
    Net(const UUID &uu, const json &, class Block &block);
    Net(const UUID &uu, const json &);
    Net(const UUID &uu);
    UUID get_uuid() const;
    UUID uuid;
    std::string name;
    bool is_power = false;

    enum class PowerSymbolStyle { GND, EARTH, DOT, ANTENNA };
    PowerSymbolStyle power_symbol_style = PowerSymbolStyle::GND;
    bool power_symbol_name_visible = true;

    uuid_ptr<class NetClass> net_class;
    uuid_ptr<Net> diffpair;
    bool diffpair_primary = false;

    bool is_port = false;
    Pin::Direction port_direction = Pin::Direction::BIDIRECTIONAL;


    // not saved
    bool is_power_forced = false;
    bool is_bussed = false;
    bool keep = false;
    unsigned int n_pins_connected = 0;
    bool has_bus_rippers = false;
    std::vector<UUIDVec> hrefs;
    bool is_nc = false;
    json serialize() const;
    bool is_named() const;
};
} // namespace horizon
