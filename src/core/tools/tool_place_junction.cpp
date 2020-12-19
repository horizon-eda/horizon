#include "tool_place_junction.hpp"
#include "document/idocument.hpp"
#include "imp/imp_interface.hpp"
#include "common/junction.hpp"
#include <iostream>

namespace horizon {

bool ToolPlaceJunctionBase::can_begin()
{
    return doc.r->has_object_type(ObjectType::JUNCTION);
}

ToolResponse ToolPlaceJunctionBase::begin(const ToolArgs &args)
{
    std::cout << "tool place junction\n";

    imp->tool_bar_set_actions({
            {InToolActionID::LMB},
            {InToolActionID::RMB},
    });

    if (!begin_attached()) {
        return ToolResponse::end();
    }

    create_junction(args.coords);
    create_attached();

    return ToolResponse();
}

void ToolPlaceJunction::insert_junction()
{
    temp = doc.r->insert_junction(UUID::random());
}

void ToolPlaceJunctionBase::create_junction(const Coordi &c)
{
    insert_junction();
    imp->set_snap_filter({{ObjectType::JUNCTION, get_junction()->uuid}});
    get_junction()->position = c;
}

ToolResponse ToolPlaceJunctionBase::update(const ToolArgs &args)
{
    if (update_attached(args)) {
        return ToolResponse();
    }

    if (args.type == ToolEventType::MOVE) {
        get_junction()->position = args.coords;
        return ToolResponse();
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB:
            if (junction_placed()) {
                return ToolResponse();
            }
            junctions_placed.push_front(get_junction());

            create_junction(args.coords);
            create_attached();
            break;

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            delete_attached();
            doc.r->delete_junction(get_junction()->uuid);
            selection.clear();
            for (auto it : junctions_placed) {
                selection.emplace(it->uuid, ObjectType::JUNCTION);
            }
            return ToolResponse::commit();

        default:;
        }
    }
    return ToolResponse();
}
} // namespace horizon
