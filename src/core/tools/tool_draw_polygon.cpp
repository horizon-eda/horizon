#include "tool_draw_polygon.hpp"
#include "document/idocument.hpp"
#include "imp/imp_interface.hpp"
#include "util/geom_util.hpp"
#include <iostream>
#include <sstream>

namespace horizon {

bool ToolDrawPolygon::can_begin()
{
    return doc.r->has_object_type(ObjectType::POLYGON);
}

ToolResponse ToolDrawPolygon::begin(const ToolArgs &args)
{
    std::cout << "tool draw line poly\n";

    temp = doc.r->insert_polygon(UUID::random());
    temp->layer = args.work_layer;
    append_vertex(args.coords);

    update_tip();
    return ToolResponse();
}

void ToolDrawPolygon::append_vertex(const Coordi &c)
{
    if (last_vertex && last_vertex->position == vertex->position) {
        imp->tool_bar_flash("didn't create zero-length edge");
        return;
    }
    vertex = temp->append_vertex();
    if (temp->vertices.size() >= 2)
        last_vertex = &temp->vertices.at(temp->vertices.size() - 2);
    else
        last_vertex = nullptr;
    set_snap_filter();
    cycle_restrict_mode_xy();
    if (last_vertex)
        vertex->position = get_coord_restrict(last_vertex->position, c);
    else
        vertex->position = c;
    selection = {{temp->uuid, ObjectType::POLYGON_VERTEX, static_cast<unsigned int>(temp->vertices.size() - 1)}};
}

void ToolDrawPolygon::set_snap_filter()
{
    if (arc_mode == ArcMode::CURRENT)
        imp->set_snap_filter({
                {ObjectType::POLYGON_ARC_CENTER, temp->uuid, static_cast<int>(temp->vertices.size() - 2)},
                {ObjectType::POLYGON_EDGE, temp->uuid},
        });
    else
        imp->set_snap_filter({
                {ObjectType::POLYGON_VERTEX, temp->uuid, static_cast<int>(temp->vertices.size() - 1)},
                {ObjectType::POLYGON_EDGE, temp->uuid},
        });
}

void ToolDrawPolygon::update_tip()
{
    std::vector<ActionLabelInfo> actions;
    actions.reserve(9);
    if (arc_mode == ArcMode::CURRENT) {
        actions.emplace_back(InToolActionID::LMB, "place arc enter");
    }
    else if (arc_mode == ArcMode::NEXT) {
        actions.emplace_back(InToolActionID::LMB, "place vertex, then arc enter");
    }
    else {
        actions.emplace_back(InToolActionID::LMB, "place vertex");
    }
    actions.emplace_back(InToolActionID::RMB, "delete last vertex and finish");
    actions.emplace_back(InToolActionID::TOGGLE_ARC);
    actions.emplace_back(InToolActionID::RESTRICT);
    actions.emplace_back(InToolActionID::TOGGLE_DEG45_RESTRICT);
    if (last_vertex && (last_vertex->type == Polygon::Vertex::Type::ARC)) {
        actions.emplace_back(InToolActionID::FLIP_ARC);
    }
    imp->tool_bar_set_actions(actions);
    imp->tool_bar_set_tip(restrict_mode_to_string());
}

void ToolDrawPolygon::update_vertex(const Coordi &c)
{
    if (arc_mode == ArcMode::CURRENT && last_vertex) {
        const auto p = project_onto_perp_bisector(last_vertex->position, vertex->position, c);
        last_vertex->arc_center = Coordi(p.x, p.y);
    }
    else {
        if (last_vertex == nullptr) {
            vertex->position = c;
        }
        else {
            vertex->position = get_coord_restrict(last_vertex->position, c);
        }
    }
}

ToolResponse ToolDrawPolygon::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        update_vertex(args.coords);
        return ToolResponse();
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB:
            if (arc_mode == ArcMode::CURRENT) {
                arc_mode = ArcMode::OFF;
                append_vertex(args.coords);
            }
            else if (arc_mode == ArcMode::NEXT) {
                arc_mode = ArcMode::CURRENT;
                selection = {{temp->uuid, ObjectType::POLYGON_ARC_CENTER,
                              static_cast<unsigned int>(temp->vertices.size() - 2)}};
                last_vertex->type = Polygon::Vertex::Type::ARC;
                last_vertex->arc_center = args.coords;
                set_snap_filter();
            }
            else {
                if (temp->vertices.size() > 3 && temp->vertices.front().position == vertex->position) {
                    // closed cycle
                    temp->vertices.pop_back();
                    return commit();
                }

                append_vertex(args.coords);
            }
            break;

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            temp->vertices.pop_back();
            vertex = nullptr;
            if (!temp->is_valid()) {
                return ToolResponse::revert();
            }
            return commit();
            break;

        case InToolActionID::TOGGLE_ARC:
            if (last_vertex) {
                if (arc_mode == ArcMode::OFF) {
                    arc_mode = ArcMode::NEXT;
                }
                else if (arc_mode == ArcMode::NEXT) {
                    arc_mode = ArcMode::OFF;
                }
                set_snap_filter();
            }
            break;

        case InToolActionID::FLIP_ARC:
            if (last_vertex && (last_vertex->type == Polygon::Vertex::Type::ARC)) {
                last_vertex->arc_reverse ^= 1;
            }
            break;

        case InToolActionID::RESTRICT:
            cycle_restrict_mode();
            update_vertex(args.coords);
            break;
        case InToolActionID::TOGGLE_DEG45_RESTRICT:
            toogle_45_degrees_mode();
            update_vertex(args.coords);
            break;
        default:;
        }
    }
    else if (args.type == ToolEventType::LAYER_CHANGE) {
        temp->layer = args.work_layer;
    }
    update_tip();
    return ToolResponse();
}

ToolResponse ToolDrawPolygon::commit()
{
    return ToolResponse::commit();
}

} // namespace horizon
