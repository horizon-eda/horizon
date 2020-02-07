#include "tool_draw_polygon.hpp"
#include "idocument_padstack.hpp"
#include "pool/padstack.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>
#include <gdk/gdkkeysyms.h>

namespace horizon {

ToolDrawPolygon::ToolDrawPolygon(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolDrawPolygon::can_begin()
{
    return doc.r->has_object_type(ObjectType::POLYGON);
}

ToolResponse ToolDrawPolygon::begin(const ToolArgs &args)
{
    std::cout << "tool draw line poly\n";

    temp = doc.r->insert_polygon(UUID::random());
    temp->temp = true;
    temp->layer = args.work_layer;
    vertex = temp->append_vertex();
    vertex->position = args.coords;

    update_tip();
    return ToolResponse();
}

void ToolDrawPolygon::update_tip()
{
    std::stringstream ss;
    ss << "<b>LMB:</b>";
    if (arc_mode == ArcMode::CURRENT) {
        ss << "place arc center";
    }
    else if (arc_mode == ArcMode::NEXT) {
        ss << "place vertex, then arc center";
    }
    else {
        ss << "place vertex";
    }

    ss << " <b>RMB:</b>delete last vertex and finish <b>a:</b> make the "
          "current edge an arc ";
    if (last_vertex && (last_vertex->type == Polygon::Vertex::Type::ARC)) {
        ss << "<b>e:</b> reverse arc direction";
    }
    else {
        ss << "<b>/:</b>restrict <i>";
        ss << restrict_mode_to_string();
        ss << "</i>";
    }
    imp->tool_bar_set_tip(ss.str());
}

void ToolDrawPolygon::update_vertex(const Coordi &c)
{
    if (arc_mode == ArcMode::CURRENT && last_vertex) {
        last_vertex->arc_center = c;
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
        return ToolResponse::fast();
    }
    else if (args.type == ToolEventType::CLICK) {
        if (args.button == 1) {
            if (arc_mode == ArcMode::CURRENT) {
                arc_mode = ArcMode::OFF;
                last_vertex = vertex;
                vertex = temp->append_vertex();
                cycle_restrict_mode_xy();
                vertex->position = get_coord_restrict(last_vertex->position, args.coords);
            }
            else if (arc_mode == ArcMode::NEXT) {
                arc_mode = ArcMode::CURRENT;
                last_vertex->type = Polygon::Vertex::Type::ARC;
                last_vertex->arc_center = args.coords;
            }
            else {
                last_vertex = vertex;
                vertex = temp->append_vertex();
                cycle_restrict_mode_xy();
                vertex->position = get_coord_restrict(last_vertex->position, args.coords);
            }
        }
        else if (args.button == 3) {
            temp->vertices.pop_back();
            temp->temp = false;
            vertex = nullptr;
            if (!temp->is_valid()) {
                doc.r->delete_polygon(temp->uuid);
            }
            return ToolResponse::commit();
        }
    }
    else if (args.type == ToolEventType::LAYER_CHANGE) {
        temp->layer = args.work_layer;
    }
    else if (args.type == ToolEventType::KEY) {
        if (args.key == GDK_KEY_a) {
            if (last_vertex) {
                if (arc_mode == ArcMode::OFF) {
                    arc_mode = ArcMode::NEXT;
                }
                else if (arc_mode == ArcMode::NEXT) {
                    arc_mode = ArcMode::OFF;
                }
            }
        }
        else if (args.key == GDK_KEY_e) {
            if (last_vertex && (last_vertex->type == Polygon::Vertex::Type::ARC)) {
                last_vertex->arc_reverse ^= 1;
            }
        }
        else if (args.key == GDK_KEY_slash) {
            cycle_restrict_mode();
            update_vertex(args.coords);
        }
        else if (args.key == GDK_KEY_Escape) {
            return ToolResponse::revert();
        }
    }
    update_tip();
    return ToolResponse();
}
} // namespace horizon
