#include "tool_draw_line_circle.hpp"
#include "common/arc.hpp"
#include "imp/imp_interface.hpp"
#include <sstream>
#include "document/idocument.hpp"

namespace horizon {

ToolDrawLineCircle::ToolDrawLineCircle(IDocument *c, ToolID tid) : ToolHelperLineWidthSetting(c, tid)
{
}

bool ToolDrawLineCircle::can_begin()
{
    return doc.r->has_object_type(ObjectType::ARC);
}

void ToolDrawLineCircle::apply_settings()
{
    for (auto &arc : arcs) {
        if (arc)
            arc->width = settings.width;
    }
}

void ToolDrawLineCircle::update_junctions()
{
    if (step == 1) {
        junctions.at(0)->position = second_pos;
        junctions.at(1)->position = first_pos;
        junctions.at(2)->position = first_pos - (second_pos - first_pos);
    }
}

void ToolDrawLineCircle::create()
{
    for (auto &it : junctions) {
        it = doc.r->insert_junction(UUID::random());
    }
    for (auto &it : arcs) {
        it = doc.r->insert_arc(UUID::random());
        it->center = junctions.at(1);
        it->layer = imp->get_work_layer();
    }
    arcs.at(0)->from = junctions.at(0);
    arcs.at(0)->to = junctions.at(2);
    arcs.at(1)->from = junctions.at(2);
    arcs.at(1)->to = junctions.at(0);
    apply_settings();
}

ToolResponse ToolDrawLineCircle::begin(const ToolArgs &args)
{
    update_tip();
    return ToolResponse();
}

void ToolDrawLineCircle::update_tip()
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
        actions.emplace_back(InToolActionID::ENTER_WIDTH, "line width");
        actions.emplace_back(InToolActionID::ENTER_DATUM, "set radius and finish");
    }

    imp->tool_bar_set_actions(actions);
}

ToolResponse ToolDrawLineCircle::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        if (step == 0) {
            first_pos = args.coords;
        }
        else if (step == 1) {
            second_pos = args.coords;
            update_junctions();
        }
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB:
            if (step == 0) {
                step = 1;
                second_pos = args.coords;
                create();
                update_junctions();
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
                if (auto r = imp->dialogs.ask_datum("Radius", 1_mm)) {
                    second_pos = first_pos + Coordi(*r, 0);
                    update_junctions();
                    return ToolResponse::commit();
                }
            }
            break;

        case InToolActionID::ENTER_WIDTH:
            ask_line_width();
            break;

        default:;
        }
    }

    else if (args.type == ToolEventType::LAYER_CHANGE) {
        if (step == 1) {
            for (auto arc : arcs) {
                arc->layer = args.work_layer;
            }
        }
    }
    update_tip();
    return ToolResponse();
}
} // namespace horizon
