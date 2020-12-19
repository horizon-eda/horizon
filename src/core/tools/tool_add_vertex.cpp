#include "tool_add_vertex.hpp"
#include "common/polygon.hpp"
#include "imp/imp_interface.hpp"
#include "document/idocument.hpp"

namespace horizon {

bool ToolAddVertex::can_begin()
{
    return std::count_if(selection.begin(), selection.end(),
                         [](const auto &x) { return x.type == ObjectType::POLYGON_EDGE; })
           == 1;
}

void ToolAddVertex::update_tip()
{
    std::vector<ActionLabelInfo> actions = {
            {InToolActionID::LMB, "place vertex"},
            {InToolActionID::RMB, "cancel"},
    };
    if (n_vertices_placed) {
        actions.emplace_back(InToolActionID::FLIP_DIRECTION);
    }
    imp->tool_bar_set_actions(actions);
}

ToolResponse ToolAddVertex::begin(const ToolArgs &args)
{
    int v = 0;
    for (const auto &it : args.selection) {
        if (it.type == ObjectType::POLYGON_EDGE) {
            poly = doc.r->get_polygon(it.uuid);
            v = it.vertex;
            break;
        }
    }

    if (!poly)
        return ToolResponse::end();

    vertex_index = (v + 1) % poly->vertices.size();
    vertex = &*poly->vertices.insert(poly->vertices.begin() + vertex_index, args.coords);
    selection.clear();
    selection.emplace(poly->uuid, ObjectType::POLYGON_VERTEX, vertex_index);
    update_tip();
    return ToolResponse();
}

void ToolAddVertex::add_vertex(const Coordi &c)
{
    vertex = &*poly->vertices.insert(poly->vertices.begin() + vertex_index, c);
    selection.clear();
    selection.emplace(poly->uuid, ObjectType::POLYGON_VERTEX, vertex_index);
}

ToolResponse ToolAddVertex::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        vertex->position = args.coords;
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB: {
            if (flip_direction)
                vertex_index = (vertex_index + 1) % (poly->vertices.size());
            add_vertex(args.coords);
            n_vertices_placed++;
            update_tip();
        } break;

        case InToolActionID::FLIP_DIRECTION:
            if (n_vertices_placed > 0) {
                poly->vertices.erase(poly->vertices.begin() + vertex_index);
                if (flip_direction) {
                    if (vertex_index)
                        vertex_index--;
                    else
                        vertex_index = poly->vertices.size() - 1;
                }
                else {
                    vertex_index = (vertex_index + 1) % (poly->vertices.size());
                }
                add_vertex(args.coords);
                flip_direction ^= true;
            }
            break;

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            if (n_vertices_placed == 0) {
                selection.clear();
                return ToolResponse::revert();
            }
            else {
                poly->vertices.erase(poly->vertices.begin() + vertex_index);
                return ToolResponse::commit();
            }

        default:;
        }
    }
    return ToolResponse();
}
} // namespace horizon
