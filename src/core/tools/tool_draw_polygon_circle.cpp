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

    std::vector<ActionLabelInfo> actions;
    actions.reserve(8);

    if (step == 0) {
        actions.emplace_back(InToolActionID::LMB, "place center");
    }
    else {
        actions.emplace_back(InToolActionID::LMB, "place radius");
    }
    actions.emplace_back(InToolActionID::RMB, "cancel");
    if (step == 1) {
        actions.emplace_back(InToolActionID::ENTER_DATUM, "set radius and finish");
    }

    imp->tool_bar_set_actions(actions);
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
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB:
            if (step == 0) {
                step = 1;
            }
            else {
                return ToolResponse::commit();
            }
            break;

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            return ToolResponse::revert();

        case InToolActionID::ENTER_DATUM:
            if (step == 1) {
                auto r = imp->dialogs.ask_datum("Radius", 1_mm);
                if (r.first) {
                    second_pos = first_pos + Coordi(r.second, 0);
                    update_polygon();
                    return ToolResponse::commit();
                }
            }
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
} // namespace horizon
