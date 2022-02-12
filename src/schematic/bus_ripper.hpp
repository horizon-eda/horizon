#pragma once
#include "util/uuid.hpp"
#include "nlohmann/json_fwd.hpp"
#include "common/common.hpp"
#include "util/uuid_ptr.hpp"
#include "block/bus.hpp"
#include <vector>

namespace horizon {
using json = nlohmann::json;

/**
 * Make a Bus member's Net available on the schematic.
 */
class BusRipper {
public:
    BusRipper(const UUID &uu, const json &j, class Sheet &sheet, class Block &block);
    BusRipper(const UUID &uu, const json &j);
    BusRipper(const UUID &uu);
    UUID get_uuid() const;
    UUID uuid;

    uuid_ptr<class SchematicJunction> junction;
    Orientation orientation = Orientation::UP;
    void mirror();
    uuid_ptr<Bus> bus = nullptr;
    uuid_ptr<Bus::Member> bus_member = nullptr;
    std::vector<UUID> connections;

    void update_refs(class Sheet &sheet, class Block &block);
    Coordi get_connector_pos() const;


    json serialize() const;

    UUID net_segment; // net segment from net, not from bus
};
} // namespace horizon
