#include "tool_place_board_panel.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>

namespace horizon {

bool ToolPlaceBoardPanel::can_begin()
{
    return doc.b && doc.b->get_board()->included_boards.size();
}

ToolResponse ToolPlaceBoardPanel::begin(const ToolArgs &args)
{
    if (auto r = imp->dialogs.select_included_board(*doc.b->get_board())) {
        inc_board = &doc.b->get_board()->included_boards.at(*r);
        create_board(args.coords);

        imp->tool_bar_set_actions({
                {InToolActionID::LMB},
                {InToolActionID::RMB},
                {InToolActionID::ROTATE},
        });
        return ToolResponse();
    }
    else {
        return ToolResponse::end();
    }
}

void ToolPlaceBoardPanel::create_board(const Coordi &pos)
{
    Board *brd = doc.b->get_board();
    auto uu = UUID::random();
    temp = &brd->board_panels
                    .emplace(std::piecewise_construct, std::forward_as_tuple(uu), std::forward_as_tuple(uu, *inc_board))
                    .first->second;
    temp->placement.shift = pos;
}

ToolResponse ToolPlaceBoardPanel::update(const ToolArgs &args)
{

    if (args.type == ToolEventType::MOVE) {
        temp->placement.shift = args.coords;
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB:
            create_board(args.coords);
            break;

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            doc.b->get_board()->board_panels.erase(temp->uuid);
            temp = 0;
            selection.clear();
            return ToolResponse::commit();

        case InToolActionID::ROTATE:
            temp->placement.inc_angle_deg(-90);
            break;

        default:;
        }
    }
    return ToolResponse();
}
} // namespace horizon
