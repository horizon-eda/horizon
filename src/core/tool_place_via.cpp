#include "tool_place_via.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>

namespace horizon {

ToolPlaceVia::ToolPlaceVia(IDocument *c, ToolID tid) : ToolBase(c, tid), ToolPlaceJunction(c, tid)
{
}

bool ToolPlaceVia::can_begin()
{
    return doc.b;
}

bool ToolPlaceVia::begin_attached()
{
    bool r;
    UUID ps_uuid;
    std::tie(r, ps_uuid) = imp->dialogs.select_via_padstack(doc.b->get_via_padstack_provider());
    if (!r) {
        return false;
    }
    padstack = doc.b->get_via_padstack_provider()->get_padstack(ps_uuid);
    imp->tool_bar_set_tip("<b>LMB:</b>place via <b>RMB:</b>delete current via and finish");
    return true;
}

void ToolPlaceVia::create_attached()
{
    auto uu = UUID::random();
    via = &doc.b->get_board()
                   ->vias
                   .emplace(std::piecewise_construct, std::forward_as_tuple(uu), std::forward_as_tuple(uu, padstack))
                   .first->second;
    via->junction = temp;
}

void ToolPlaceVia::delete_attached()
{
    if (via) {
        doc.b->get_board()->vias.erase(via->uuid);
    }
}

bool ToolPlaceVia::update_attached(const ToolArgs &args)
{
    if (args.type == ToolEventType::CLICK) {
        if (args.button == 1) {
            if (args.target.type == ObjectType::JUNCTION) {
                Junction *j = doc.r->get_junction(args.target.path.at(0));
                via->junction = j;
                create_attached();
                return true;
            }
        }
    }
    return false;
}
} // namespace horizon
