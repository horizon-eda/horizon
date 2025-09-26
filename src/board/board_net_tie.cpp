#include "board_net_tie.hpp"
#include "board.hpp"
#include "block/block.hpp"
#include <nlohmann/json.hpp>

namespace horizon {

BoardNetTie::BoardNetTie(const UUID &uu, const json &j, class Board *brd)
    : uuid(uu), net_tie(&brd->block->net_ties.at(j.at("net_tie").get<std::string>())),
      from(&brd->junctions.at(j.at("from").get<std::string>())), to(&brd->junctions.at(j.at("to").get<std::string>())),
      layer(j.at("layer").get<int>()), width(j.at("width").get<uint64_t>()),
      width_from_rules(j.at("width_from_rules").get<bool>())
{
}

BoardNetTie::BoardNetTie(const UUID &uu) : uuid(uu)
{
}

json BoardNetTie::serialize() const
{
    json j;
    j["net_tie"] = static_cast<std::string>(net_tie->uuid);
    j["from"] = static_cast<std::string>(from->uuid);
    j["to"] = static_cast<std::string>(to->uuid);
    j["layer"] = layer;
    j["width"] = width;
    j["width_from_rules"] = width_from_rules;
    return j;
}

void BoardNetTie::update_refs(class Board &brd)
{
    from.update(brd.junctions);
    to.update(brd.junctions);
}

UUID BoardNetTie::get_uuid() const
{
    return uuid;
}

} // namespace  horizon
