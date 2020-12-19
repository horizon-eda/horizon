#include "tool_draw_arc.hpp"
#include "imp/imp_interface.hpp"
#include "document/idocument.hpp"
#include <iostream>
#include "common/arc.hpp"
#include <sstream>

namespace horizon {

void ToolDrawArc::apply_settings()
{
    if (temp_arc)
        temp_arc->width = settings.width;
}

bool ToolDrawArc::can_begin()
{
    return doc.r->has_object_type(ObjectType::ARC);
}

Junction *ToolDrawArc::make_junction(const Coordi &coords)
{
    Junction *ju;
    ju = doc.r->insert_junction(UUID::random());
    imp->set_snap_filter({{ObjectType::JUNCTION, ju->uuid}});
    ju->position = coords;
    return ju;
}

ToolResponse ToolDrawArc::begin(const ToolArgs &args)
{
    std::cout << "tool draw arc\n";

    temp_junc = make_junction(args.coords);
    temp_arc = nullptr;
    from_junc = nullptr;
    to_junc = nullptr;
    state = DrawArcState::FROM;
    update_tip();
    return ToolResponse();
}

void ToolDrawArc::update_tip()
{
    std::vector<ActionLabelInfo> actions;
    actions.reserve(8);
    if (state == DrawArcState::FROM) {
        actions.emplace_back(InToolActionID::LMB, "place from junction");
    }
    else if (state == DrawArcState::TO) {
        actions.emplace_back(InToolActionID::LMB, "place to junction");
    }
    else if (state == DrawArcState::CENTER) {
        actions.emplace_back(InToolActionID::LMB, "place center junction");
    }
    actions.emplace_back(InToolActionID::RMB, "cancel");
    if (temp_arc)
        actions.emplace_back(InToolActionID::FLIP_ARC);
    actions.emplace_back(InToolActionID::ENTER_WIDTH, "line width");
    imp->tool_bar_set_actions(actions);
}

ToolResponse ToolDrawArc::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        temp_junc->position = args.coords;
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB:
            if (state == DrawArcState::FROM) {
                if (args.target.type == ObjectType::JUNCTION) {
                    from_junc = doc.r->get_junction(args.target.path.at(0));
                }
                else {
                    from_junc = temp_junc;
                    temp_junc = make_junction(args.coords);
                }
                state = DrawArcState::TO;
            }
            else if (state == DrawArcState::TO) {
                if (args.target.type == ObjectType::JUNCTION) {
                    to_junc = doc.r->get_junction(args.target.path.at(0));
                }
                else {
                    to_junc = temp_junc;
                    temp_junc = make_junction(args.coords);
                }
                temp_arc = doc.r->insert_arc(UUID::random());
                apply_settings();
                temp_arc->from = from_junc;
                temp_arc->to = to_junc;
                temp_arc->center = temp_junc;
                temp_arc->layer = args.work_layer;
                state = DrawArcState::CENTER;
            }
            else if (state == DrawArcState::CENTER) {
                if (args.target.type == ObjectType::JUNCTION) {
                    temp_arc->center = doc.r->get_junction(args.target.path.at(0));
                    doc.r->delete_junction(temp_junc->uuid);
                    temp_junc = nullptr;
                }
                else {
                    temp_arc->center = temp_junc;
                }
                return ToolResponse::commit();
            }
            break;

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            return ToolResponse::revert();

        case InToolActionID::ENTER_WIDTH:
            ask_line_width();
            break;

        case InToolActionID::FLIP_ARC:
            if (temp_arc) {
                temp_arc->reverse();
            }
            break;

        default:;
        }
    }
    else if (args.type == ToolEventType::LAYER_CHANGE) {
        if (temp_arc)
            temp_arc->layer = args.work_layer;
    }
    update_tip();
    return ToolResponse();
}
} // namespace horizon
