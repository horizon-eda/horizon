#include "tool_draw_line.hpp"
#include "imp/imp_interface.hpp"
#include "pool/part.hpp"
#include <algorithm>
#include <nlohmann/json.hpp>
#include "document/idocument.hpp"

namespace horizon {

bool ToolDrawLine::can_begin()
{
    return doc.r->has_object_type(ObjectType::LINE);
}

void ToolDrawLine::apply_settings()
{
    if (temp_line)
        temp_line->width = settings.width;
}

ToolResponse ToolDrawLine::begin(const ToolArgs &args)
{
    temp_junc = doc.r->insert_junction(UUID::random());
    imp->set_snap_filter({{ObjectType::JUNCTION, temp_junc->uuid}});
    junctions_created.insert(temp_junc);
    temp_junc->position = args.coords;
    temp_line = nullptr;
    update_tip();

    return ToolResponse();
}

void ToolDrawLine::update_tip()
{
    std::vector<ActionLabelInfo> actions;
    actions.reserve(9);
    actions.emplace_back(InToolActionID::LMB, "place junction");
    if (temp_line) {
        actions.emplace_back(InToolActionID::RMB, "finish current segment");
    }
    else {
        actions.emplace_back(InToolActionID::RMB, "end tool");
    }
    actions.emplace_back(InToolActionID::ENTER_WIDTH, "line width");
    actions.emplace_back(InToolActionID::RESTRICT);
    actions.emplace_back(InToolActionID::TOGGLE_DEG45_RESTRICT);

    imp->tool_bar_set_tip(restrict_mode_to_string());
    imp->tool_bar_set_actions(actions);
}

void ToolDrawLine::do_move(const Coordi &c)
{
    if (temp_line == nullptr) {
        temp_junc->position = c;
    }
    else {
        temp_junc->position = get_coord_restrict(temp_line->from->position, c);
    }
}

ToolResponse ToolDrawLine::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        do_move(args.coords);
        update_tip();
        return ToolResponse();
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB:
            if (args.target.type == ObjectType::JUNCTION
                && (temp_line == nullptr
                    || get_coord_restrict(temp_line->from->position, args.coords) == args.coords)) {
                if (temp_line != nullptr) {
                    temp_line->to = doc.r->get_junction(args.target.path.at(0));
                }
                if (temp_line)
                    first_line = false;
                temp_line = doc.r->insert_line(UUID::random());
                temp_line->from = doc.r->get_junction(args.target.path.at(0));
            }
            else {
                Junction *last = temp_junc;
                temp_junc = doc.r->insert_junction(UUID::random());
                imp->set_snap_filter({{ObjectType::JUNCTION, temp_junc->uuid}});
                junctions_created.insert(temp_junc);
                cycle_restrict_mode_xy();
                temp_junc->position = get_coord_restrict(last->position, args.coords);

                if (temp_line)
                    first_line = false;
                temp_line = doc.r->insert_line(UUID::random());
                temp_line->from = last;
            }
            temp_line->layer = args.work_layer;
            temp_line->width = settings.width;
            temp_line->to = temp_junc;
            break;

        case InToolActionID::RMB:
            if (temp_line) {
                if (first_line && junctions_created.count(temp_line->from))
                    doc.r->delete_junction(temp_line->from->uuid);
                doc.r->delete_line(temp_line->uuid);
                temp_line = nullptr;
                first_line = true;
            }
            else {
                doc.r->delete_junction(temp_junc->uuid);
                temp_junc = nullptr;
                return ToolResponse::commit();
            }
            break;

        case InToolActionID::ENTER_WIDTH:
            ask_line_width();
            break;

        case InToolActionID::RESTRICT:
            cycle_restrict_mode();
            do_move(args.coords);
            break;

        case InToolActionID::TOGGLE_DEG45_RESTRICT:
            toogle_45_degrees_mode();
            do_move(args.coords);
            break;

        case InToolActionID::CANCEL:
            if (temp_line) {
                if (first_line && junctions_created.count(temp_line->from))
                    doc.r->delete_junction(temp_line->from->uuid);
                doc.r->delete_line(temp_line->uuid);
                temp_line = nullptr;
            }
            doc.r->delete_junction(temp_junc->uuid);
            temp_junc = nullptr;
            return ToolResponse::commit();

        default:;
        }
    }
    else if (args.type == ToolEventType::LAYER_CHANGE) {
        if (temp_line)
            temp_line->layer = args.work_layer;
    }

    update_tip();
    return ToolResponse();
}
} // namespace horizon
