#include "schematic_net_tie.hpp"
#include "sheet.hpp"
#include "block/block.hpp"
#include <nlohmann/json.hpp>

namespace horizon {

SchematicNetTie::SchematicNetTie(const UUID &uu, const json &j, class Sheet &sheet, class Block &block)
    : uuid(uu), net_tie(&block.net_ties.at(j.at("net_tie").get<std::string>())),
      from(&sheet.junctions.at(j.at("from").get<std::string>())), to(&sheet.junctions.at(j.at("to").get<std::string>()))
{
}

SchematicNetTie::SchematicNetTie(const UUID &uu) : uuid(uu)
{
}

json SchematicNetTie::serialize() const
{
    json j;
    j["net_tie"] = static_cast<std::string>(net_tie->uuid);
    j["from"] = static_cast<std::string>(from->uuid);
    j["to"] = static_cast<std::string>(to->uuid);
    return j;
}

void SchematicNetTie::update_refs(class Sheet &sheet)
{
    from.update(sheet.junctions);
    to.update(sheet.junctions);
}

UUID SchematicNetTie::get_uuid() const
{
    return uuid;
}

} // namespace  horizon
