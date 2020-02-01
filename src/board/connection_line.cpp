#include "connection_line.hpp"
#include "board.hpp"
#include "board_layers.hpp"
#include "board_package.hpp"
#include "common/lut.hpp"
#include "nlohmann/json.hpp"

namespace horizon {

ConnectionLine::ConnectionLine(const UUID &uu, const json &j, Board *brd)
    : uuid(uu), from(j.at("from"), brd), to(j.at("to"), brd)
{
}

json ConnectionLine::serialize() const
{
    json j;
    j["from"] = from.serialize();
    j["to"] = to.serialize();
    return j;
}

void ConnectionLine::update_refs(Board &brd)
{
    to.update_refs(brd);
    from.update_refs(brd);
}

UUID ConnectionLine::get_uuid() const
{
    return uuid;
}

ConnectionLine::ConnectionLine(const UUID &uu) : uuid(uu)
{
}
} // namespace horizon
