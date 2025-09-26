#include "power_symbol.hpp"
#include "common/lut.hpp"
#include "sheet.hpp"
#include <nlohmann/json.hpp>

namespace horizon {


PowerSymbol::PowerSymbol(const UUID &uu, const json &j, Sheet *sheet, Block *block)
    : uuid(uu), mirror(j.value("mirror", false))
{
    if (sheet)
        junction = &sheet->junctions.at(j.at("junction").get<std::string>());
    else
        junction.uuid = j.at("junction").get<std::string>();

    if (block)
        net = &block->nets.at(j.at("net").get<std::string>());
    else
        net.uuid = j.at("net").get<std::string>();

    if (j.count("orientation"))
        orientation = orientation_lut.lookup(j.at("orientation"));
}

void PowerSymbol::mirrorx()
{
    switch (orientation) {
    case Orientation::UP:
        mirror ^= true;
        break;
    case Orientation::RIGHT:
        orientation = Orientation::LEFT;
        break;
    case Orientation::DOWN:
        mirror ^= true;
        break;
    case Orientation::LEFT:
        orientation = Orientation::RIGHT;
        break;
    }
}

UUID PowerSymbol::get_uuid() const
{
    return uuid;
}

void PowerSymbol::update_refs(Sheet &sheet, Block &block)
{
    junction.update(sheet.junctions);
    net.update(block.nets);
}

PowerSymbol::PowerSymbol(const UUID &uu) : uuid(uu)
{
}

json PowerSymbol::serialize() const
{
    json j;
    j["junction"] = (std::string)junction->uuid;
    j["net"] = (std::string)net->uuid;
    j["mirror"] = mirror;
    j["orientation"] = orientation_lut.lookup_reverse(orientation);
    return j;
}
} // namespace horizon
