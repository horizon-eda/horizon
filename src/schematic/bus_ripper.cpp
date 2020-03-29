#include "bus_ripper.hpp"
#include "sheet.hpp"
#include "block/block.hpp"
#include "nlohmann/json.hpp"

namespace horizon {

BusRipper::BusRipper(const UUID &uu, const json &j)
    : uuid(uu), junction(UUID(j.at("junction").get<std::string>())), bus(UUID(j.at("bus").get<std::string>())),
      bus_member(UUID(j.at("bus_member").get<std::string>()))
{

    if (j.count("orientation")) {
        orientation = orientation_lut.lookup(j.at("orientation"));
    }
    else if (j.count("mirror")) {
        bool m = j.at("mirror");
        if (m)
            orientation = Orientation::LEFT;
        else
            orientation = Orientation::UP;
    }
}

BusRipper::BusRipper(const UUID &uu, const json &j, Sheet &sheet, Block &block) : BusRipper(uu, j)
{
    junction.update(sheet.junctions);
    bus.update(block.buses);
    bus_member.update(bus->members);
}

BusRipper::BusRipper(const UUID &uu) : uuid(uu)
{
}

json BusRipper::serialize() const
{
    json j;
    j["junction"] = (std::string)junction->uuid;
    j["orientation"] = orientation_lut.lookup_reverse(orientation);
    j["bus"] = (std::string)bus->uuid;
    j["bus_member"] = (std::string)bus_member->uuid;
    return j;
}

Coordi BusRipper::get_connector_pos() const
{
    Coordi offset;
    switch (orientation) {
    case Orientation::UP:
        offset = {1, 1};
        break;
    case Orientation::RIGHT:
        offset = {1, -1};
        break;
    case Orientation::DOWN:
        offset = {-1, -1};
        break;
    case Orientation::LEFT:
        offset = {-1, 1};
        break;
    }
    return junction->position + offset * 1.25_mm;
}

void BusRipper::mirror()
{
    switch (orientation) {
    case Orientation::UP:
        orientation = Orientation::LEFT;
        break;
    case Orientation::RIGHT:
        orientation = Orientation::DOWN;
        break;
    case Orientation::DOWN:
        orientation = Orientation::RIGHT;
        break;
    case Orientation::LEFT:
        orientation = Orientation::UP;
        break;
    }
}

UUID BusRipper::get_uuid() const
{
    return uuid;
}

void BusRipper::update_refs(Sheet &sheet, Block &block)
{
    junction.update(sheet.junctions);
    bus.update(block.buses);
    bus_member.update(bus->members);
}
} // namespace horizon
