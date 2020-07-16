#include "tool_place_board_hole.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>

namespace horizon {

ToolPlaceBoardHole::ToolPlaceBoardHole(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolPlaceBoardHole::can_begin()
{
    return doc.b;
}

ToolResponse ToolPlaceBoardHole::begin(const ToolArgs &args)
{
    if (auto r = imp->dialogs.select_hole_padstack(doc.r->get_pool())) {
        padstack = doc.r->get_pool()->get_padstack(*r);
        create_hole(args.coords);

        imp->tool_bar_set_actions({
                {InToolActionID::LMB},
                {InToolActionID::RMB},
        });
        return ToolResponse();
    }
    else {
        return ToolResponse::end();
    }
}

void ToolPlaceBoardHole::create_hole(const Coordi &pos)
{
    Board *brd = doc.b->get_board();
    auto uu = UUID::random();
    temp = &brd->holes.emplace(std::piecewise_construct, std::forward_as_tuple(uu), std::forward_as_tuple(uu, padstack))
                    .first->second;
    temp->placement.shift = pos;
}

ToolResponse ToolPlaceBoardHole::update(const ToolArgs &args)
{

    if (args.type == ToolEventType::MOVE) {
        temp->placement.shift = args.coords;
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB:
            create_hole(args.coords);
            break;

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            doc.b->get_board()->holes.erase(temp->uuid);
            temp = 0;
            selection.clear();
            return ToolResponse::commit();

        default:;
        }
    }
    return ToolResponse();
}
} // namespace horizon
