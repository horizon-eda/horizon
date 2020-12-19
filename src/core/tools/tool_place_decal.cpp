#include "tool_place_decal.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "imp/imp_interface.hpp"
#include "pool/ipool.hpp"

namespace horizon {

bool ToolPlaceDecal::can_begin()
{
    return doc.b;
}

ToolResponse ToolPlaceDecal::begin(const ToolArgs &args)
{
    if (auto r = imp->dialogs.select_decal(doc.b->get_pool())) {
        auto uu = UUID::random();
        temp = &doc.b->get_board()->decals.emplace(uu, uu).first->second;
        temp->pool_decal = doc.b->get_pool().get_decal(*r);
        temp->decal = *temp->pool_decal;
        temp->placement.shift = args.coords;

        imp->tool_bar_set_actions({
                {InToolActionID::LMB},
                {InToolActionID::RMB},
                {InToolActionID::ROTATE},
                {InToolActionID::MIRROR},
        });
        return ToolResponse();
    }
    else {
        return ToolResponse::end();
    }
}


ToolResponse ToolPlaceDecal::update(const ToolArgs &args)
{

    if (args.type == ToolEventType::MOVE) {
        temp->placement.shift = args.coords;
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB:
            return ToolResponse::commit();
            break;

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            return ToolResponse::revert();

        case InToolActionID::ROTATE:
            temp->placement.inc_angle_deg(-90);
            break;

        default:;
        }
    }
    return ToolResponse();
}
} // namespace horizon
