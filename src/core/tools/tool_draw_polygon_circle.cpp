#include "tool_draw_polygon_circle.hpp"
#include "common/polygon.hpp"
#include "imp/imp_interface.hpp"
#include <sstream>
#include "document/idocument.hpp"
#include <gdk/gdkkeysyms.h>

namespace horizon {

ToolDrawPolygonCircle::ToolDrawPolygonCircle(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolDrawPolygonCircle::can_begin()
{
    return doc.r->has_object_type(ObjectType::POLYGON);
}

void ToolDrawPolygonCircle::update_polygon()
{
    temp->vertices.clear();
    if (step == 1) {
        temp->vertices.emplace_back(second_pos);
        temp->vertices.emplace_back(second_pos);
        auto &x = temp->vertices.back();
        x.type = Polygon::Vertex::Type::ARC;
        x.arc_center = first_pos;
        x.arc_reverse = true;
    }
}

ToolResponse ToolDrawPolygonCircle::begin(const ToolArgs &args)
{
    temp = doc.r->insert_polygon(UUID::random());
    imp->set_snap_filter({{ObjectType::POLYGON, temp->uuid}});
    temp->layer = args.work_layer;
    first_pos = args.coords;

    update_tip();
    return ToolResponse();
}

void ToolDrawPolygonCircle::update_tip()
{
    std::stringstream ss;
    ss << "<b>LMB:</b>";
    if (step == 0) {
        ss << "place center";
    }
    else {
        ss << "place radius";
    }
    ss << " <b>RMB:</b>cancel";
    if (step == 1)
        ss << " <b>r:</b>set radius and finish";

    imp->tool_bar_set_tip(ss.str());
}

ToolResponse ToolDrawPolygonCircle::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        if (step == 0) {
            first_pos = args.coords;
        }
        else if (step == 1) {
            second_pos = args.coords;
            update_polygon();
        }
    }
    else if (args.type == ToolEventType::CLICK) {
        if (args.button == 1) {
            if (step == 0) {
                step = 1;
            }
            else {
                return ToolResponse::commit();
            }
        }
        else if (args.button == 3) {
            return ToolResponse::revert();
        }
    }
    else if (args.type == ToolEventType::LAYER_CHANGE) {
        temp->layer = args.work_layer;
    }
    else if (args.type == ToolEventType::KEY) {
        if (args.key == GDK_KEY_r && step == 1) {
            auto r = imp->dialogs.ask_datum("Radius", 1_mm);
            if (r.first) {
                second_pos = first_pos + Coordi(r.second, 0);
                update_polygon();
                return ToolResponse::commit();
            }
        }
        else if (args.key == GDK_KEY_Escape) {
            return ToolResponse::revert();
        }
    }
    update_tip();
    return ToolResponse();
}
} // namespace horizon
