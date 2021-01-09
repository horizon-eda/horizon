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
        const auto &dec = *doc.b->get_pool().get_decal(*r);
        temp = &doc.b->get_board()
                        ->decals
                        .emplace(std::piecewise_construct, std::forward_as_tuple(uu), std::forward_as_tuple(uu, dec))
                        .first->second;
        temp->placement.shift = args.coords;
        imp->set_work_layer(temp->get_layers().start());

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

        case InToolActionID::MIRROR:
            temp->set_flip(!temp->get_flip(), *doc.b->get_board());
            temp->placement.invert_angle();

            if (temp->get_flip())
                imp->set_work_layer(temp->get_layers().end());
            else
                imp->set_work_layer(temp->get_layers().start());
            break;

        default:;
        }
    }
    return ToolResponse();
}
} // namespace horizon
