#include "tool_drag_polygon_edge.hpp"
#include "common/polygon.hpp"
#include "imp/imp_interface.hpp"
#include "document/idocument_board.hpp"
#include "util/selection_util.hpp"
#include "board/board.hpp"

namespace horizon {

bool ToolDragPolygonEdge::can_begin()
{
    return sel_count_type(selection, ObjectType::POLYGON_EDGE) == 1;
}

void ToolDragPolygonEdge::update_tip()
{
    std::vector<ActionLabelInfo> actions = {
            {InToolActionID::LMB, "place edge"},
            {InToolActionID::RMB, "cancel"},
    };
    imp->tool_bar_set_actions(actions);
}

ToolDragPolygonEdge::PolyInfo::PolyInfo(const Polygon &poly, int edge)
{
    auto get_pos = [&poly](int x) { return poly.get_vertex(x).position; };
    pos_from_orig = get_pos(edge);
    pos_from2 = get_pos(edge - 1);
    pos_to_orig = get_pos(edge + 1);
    pos_to2 = get_pos(edge + 2);
}

ToolResponse ToolDragPolygonEdge::begin(const ToolArgs &args)
{

    for (const auto &it : args.selection) {
        if (it.type == ObjectType::POLYGON_EDGE) {
            poly = doc.r->get_polygon(it.uuid);
            edge = it.vertex;
            break;
        }
    }

    if (!poly)
        return ToolResponse::end();

    selection = {{poly->uuid, ObjectType::POLYGON_EDGE, edge}};
    imp->set_snap_filter({{ObjectType::POLYGON, poly->uuid}});
    poly_info.emplace(*poly, edge);
    pos_orig = args.coords;
    plane = dynamic_cast<Plane *>(poly->usage.ptr);

    if (plane) {
        plane->fragments.clear();
        plane->revision++;
    }

    update_tip();
    return ToolResponse();
}


ToolResponse ToolDragPolygonEdge::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        const auto [pos_from, pos_to] = poly_info->get_pos(args.coords - pos_orig);
        poly->get_vertex(edge).position = pos_from;
        poly->get_vertex(edge + 1).position = pos_to;
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB_RELEASE:
            if (!is_transient)
                break;
            // fall through

        case InToolActionID::LMB:
            if (doc.b && plane) {
                auto brd = doc.b->get_board();
                brd->expand_flags = static_cast<Board::ExpandFlags>(Board::EXPAND_AIRWIRES);
                brd->airwires_expand = {plane->net->uuid};
                brd->update_plane(plane);
            }
            return ToolResponse::commit();

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            return ToolResponse::revert();

        default:;
        }
    }
    return ToolResponse();
}
} // namespace horizon
