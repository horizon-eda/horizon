#include "tool_place_board_hole.hpp"
#include "core_board.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>

namespace horizon {

ToolPlaceBoardHole::ToolPlaceBoardHole(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolPlaceBoardHole::can_begin()
{
    return core.b;
}

ToolResponse ToolPlaceBoardHole::begin(const ToolArgs &args)
{
    std::cout << "tool add comp\n";
    bool r;
    UUID padstack_uuid;
    std::tie(r, padstack_uuid) = imp->dialogs.select_hole_padstack(core.r->get_pool());
    if (!r) {
        return ToolResponse::end();
    }

    padstack = core.r->get_pool()->get_padstack(padstack_uuid);
    create_hole(args.coords);

    imp->tool_bar_set_tip("<b>LMB:</b>place pad <b>RMB:</b>delete current pad and finish");
    return ToolResponse();
}

void ToolPlaceBoardHole::create_hole(const Coordi &pos)
{
    Board *brd = core.b->get_board();
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
    else if (args.type == ToolEventType::CLICK) {
        if (args.button == 1) {
            create_hole(args.coords);
        }
        else if (args.button == 3) {
            core.b->get_board()->holes.erase(temp->uuid);
            temp = 0;
            selection.clear();
            return ToolResponse::commit();
        }
    }
    else if (args.type == ToolEventType::KEY) {
        if (args.key == GDK_KEY_Escape) {
            return ToolResponse::revert();
        }
    }
    return ToolResponse();
}
} // namespace horizon
