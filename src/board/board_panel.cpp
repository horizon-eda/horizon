#include "board_panel.hpp"
#include "nlohmann/json.hpp"
#include "board.hpp"

namespace horizon {
BoardPanel::BoardPanel(const UUID &uu, const json &j, const Board &brd)
    : uuid(uu), included_board(&brd.included_boards.at(UUID(j.at("included_board").get<std::string>()))),
      placement(j.at("placement")), omit_outline(j.value("omit_outline", false))
{
}
BoardPanel::BoardPanel(const UUID &uu, const IncludedBoard &inc) : uuid(uu), included_board(&inc)
{
}


json BoardPanel::serialize() const
{
    json j;
    j["included_board"] = (std::string)included_board->uuid;
    j["placement"] = placement.serialize();
    j["omit_outline"] = omit_outline;
    return j;
}

} // namespace horizon
