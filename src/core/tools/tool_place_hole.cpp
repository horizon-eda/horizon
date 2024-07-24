#include "tool_place_hole.hpp"
#include "document/idocument_padstack.hpp"
#include "pool/padstack.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>
#include "core/tool_id.hpp"

namespace horizon {

bool ToolPlaceHole::can_begin()
{
    return doc.a;
}

ToolResponse ToolPlaceHole::begin(const ToolArgs &args)
{
    std::cout << "tool place hole\n";

    create_hole(args.coords);

    imp->tool_bar_set_actions({
            {InToolActionID::LMB},
            {InToolActionID::RMB},
    });
    return ToolResponse();
}

void ToolPlaceHole::create_hole(const Coordi &c)
{
    const auto uu = UUID::random();
    temp = &doc.a->get_padstack().holes.emplace(uu, uu).first->second;
    temp->placement.shift = c;
    if (tool_id == ToolID::PLACE_HOLE_SLOT) {
        temp->shape = Hole::Shape::SLOT;
        temp->length = 1_mm;
    }
}

ToolResponse ToolPlaceHole::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        temp->placement.shift = args.coords;
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB:
            holes_placed.push_front(temp);
            create_hole(args.coords);
            break;

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            doc.a->get_padstack().holes.erase(temp->uuid);
            temp = 0;
            selection.clear();
            for (auto it : holes_placed) {
                selection.emplace(it->uuid, ObjectType::HOLE);
            }
            return ToolResponse::commit();

        default:;
        }
    }
    return ToolResponse();
}
} // namespace horizon
