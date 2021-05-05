#include "tool_place_dot.hpp"
#include "common/polygon.hpp"
#include "imp/imp_interface.hpp"
#include <sstream>
#include "document/idocument.hpp"

namespace horizon {

bool ToolPlaceDot::can_begin()
{
    return doc.r->has_object_type(ObjectType::POLYGON);
}

const int64_t dot_radius = 0.35_mm / 2;

void ToolPlaceDot::create_polygon(const Coordi &c)
{
    temp = doc.r->insert_polygon(UUID::random());
    temp->vertices.emplace_back();
    temp->vertices.emplace_back();
    auto &x = temp->vertices.back();
    x.type = Polygon::Vertex::Type::ARC;
    x.arc_reverse = true;
    temp->layer = 0;

    imp->set_snap_filter({{ObjectType::POLYGON, temp->uuid}});
    update_polygon(c);
}

void ToolPlaceDot::update_polygon(const Coordi &c)
{
    const auto p = c + Coordi(dot_radius, 0);
    temp->vertices.at(0).position = p;
    temp->vertices.at(1).position = p;
    temp->vertices.at(1).arc_center = c;
}

ToolResponse ToolPlaceDot::begin(const ToolArgs &args)
{
    create_polygon(args.coords);

    imp->tool_bar_set_actions({
            {InToolActionID::LMB},
            {InToolActionID::RMB},
    });

    return ToolResponse();
}

ToolResponse ToolPlaceDot::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        update_polygon(args.coords);
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB:
            create_polygon(args.coords);
            break;

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            doc.r->delete_polygon(temp->uuid);
            temp = nullptr;
            return ToolResponse::commit();

        default:;
        }
    }


    return ToolResponse();
}
} // namespace horizon
