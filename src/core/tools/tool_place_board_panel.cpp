#include "tool_place_board_panel.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>
#include <gdk/gdkkeysyms.h>

namespace horizon {

ToolPlaceBoardPanel::ToolPlaceBoardPanel(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolPlaceBoardPanel::can_begin()
{
    return doc.b && doc.b->get_board()->included_boards.size();
}

ToolResponse ToolPlaceBoardPanel::begin(const ToolArgs &args)
{
    bool r;
    UUID board_uuid;
    std::tie(r, board_uuid) = imp->dialogs.select_included_board(*doc.b->get_board());
    if (!r) {
        return ToolResponse::end();
    }

    inc_board = &doc.b->get_board()->included_boards.at(board_uuid);
    create_board(args.coords);

    imp->tool_bar_set_tip("<b>LMB:</b>place board <b>RMB:</b>delete current board and finish <b>r:</b>rotate");
    return ToolResponse();
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
    else if (args.type == ToolEventType::CLICK) {
        if (args.button == 1) {
            create_board(args.coords);
        }
        else if (args.button == 3) {
            doc.b->get_board()->board_panels.erase(temp->uuid);
            temp = 0;
            selection.clear();
            return ToolResponse::commit();
        }
    }
    else if (args.type == ToolEventType::KEY) {
        if (args.key == GDK_KEY_r) {
            temp->placement.inc_angle_deg(-90);
        }
        else if (args.key == GDK_KEY_Escape) {
            return ToolResponse::revert();
        }
    }
    return ToolResponse();
}
} // namespace horizon
