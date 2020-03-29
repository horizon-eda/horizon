#include "bus_label.hpp"
#include "common/lut.hpp"
#include "sheet.hpp"
#include "block/block.hpp"
#include "nlohmann/json.hpp"

namespace horizon {
BusLabel::BusLabel(const UUID &uu, const json &j, Sheet &sheet, Block &block) : BusLabel(uu, j)
{
    junction.update(sheet.junctions);
    bus.update(block.buses);
}

BusLabel::BusLabel(const UUID &uu, const json &j)
    : uuid(uu), junction(UUID(j.at("junction").get<std::string>())),
      orientation(orientation_lut.lookup(j.at("orientation"))), size(j.value("size", 2500000)),
      offsheet_refs(j.value("offsheet_refs", true)), bus(UUID(j.at("bus").get<std::string>()))
{
}

BusLabel::BusLabel(const UUID &uu) : uuid(uu)
{
}

json BusLabel::serialize() const
{
    json j;
    j["junction"] = (std::string)junction->uuid;
    j["orientation"] = orientation_lut.lookup_reverse(orientation);
    j["size"] = size;
    j["offsheet_refs"] = offsheet_refs;
    j["bus"] = (std::string)bus->uuid;
    return j;
}
} // namespace horizon
