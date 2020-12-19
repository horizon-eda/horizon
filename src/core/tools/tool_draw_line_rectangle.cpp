#include "tool_draw_line_rectangle.hpp"
#include "common/polygon.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>
#include <sstream>
#include "common/line.hpp"
#include "document/idocument.hpp"

namespace horizon {

void ToolDrawLineRectangle::apply_settings()
{
    for (auto &li : lines) {
        li->width = settings.width;
    }
}

bool ToolDrawLineRectangle::can_begin()
{
    return doc.r->has_object_type(ObjectType::LINE);
}

void ToolDrawLineRectangle::update_junctions()
{
    if (step == 1) {
        Coordi p0t, p1t;
        if (mode == Mode::CORNER) {
            p0t = first_pos;
            p1t = second_pos;
        }
        else {
            auto &center = first_pos;
            auto a = second_pos - center;
            p0t = center - a;
            p1t = second_pos;
        }
        Coordi p0 = Coordi::min(p0t, p1t);
        Coordi p1 = Coordi::max(p0t, p1t);
        junctions[0]->position = p0;
        junctions[1]->position = {p0.x, p1.y};
        junctions[2]->position = p1;
        junctions[3]->position = {p1.x, p0.y};
    }
}

ToolResponse ToolDrawLineRectangle::begin(const ToolArgs &args)
{
    std::cout << "tool draw line\n";

    std::set<SnapFilter> sf;
    for (int i = 0; i < 4; i++) {
        junctions[i] = doc.r->insert_junction(UUID::random());
        sf.emplace(ObjectType::JUNCTION, junctions[i]->uuid);
    }
    imp->set_snap_filter(sf);

    for (int i = 0; i < 4; i++) {
        auto line = doc.r->insert_line(UUID::random());
        lines.insert(line);
        line->layer = args.work_layer;
        line->from = junctions[i];
        line->to = junctions[(i + 1) % 4];
    }
    first_pos = args.coords;
    apply_settings();
    update_tip();
    return ToolResponse();
}

void ToolDrawLineRectangle::update_tip()
{
    std::vector<ActionLabelInfo> actions;
    actions.reserve(8);

    if (mode == Mode::CENTER) {
        if (step == 0) {
            actions.emplace_back(InToolActionID::LMB, "place center");
        }
        else {
            actions.emplace_back(InToolActionID::LMB, "place corner");
        }
    }
    else {
        if (step == 0) {
            actions.emplace_back(InToolActionID::LMB, "place first corner");
        }
        else {
            actions.emplace_back(InToolActionID::LMB, "place second corner");
        }
    }
    actions.emplace_back(InToolActionID::RMB, "cancel");
    actions.emplace_back(InToolActionID::RECTANGLE_MODE, "switch mode");
    actions.emplace_back(InToolActionID::ENTER_WIDTH, "line width");
    imp->tool_bar_set_actions(actions);

    if (mode == Mode::CENTER) {
        imp->tool_bar_set_tip("from center");
    }
    else {
        imp->tool_bar_set_tip("corners");
    }
}

ToolResponse ToolDrawLineRectangle::update(const ToolArgs &args)
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
            }
            else {
                for (int i = 0; i < 4; i++) {
                }
                return ToolResponse::commit();
            }
            break;

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            return ToolResponse::revert();

        case InToolActionID::RECTANGLE_MODE:
            mode = mode == Mode::CENTER ? Mode::CORNER : Mode::CENTER;
            update_junctions();
            break;

        case InToolActionID::ENTER_WIDTH:
            ask_line_width();
            break;

        default:;
        }
    }
    else if (args.type == ToolEventType::LAYER_CHANGE) {
        for (auto it : lines) {
            it->layer = args.work_layer;
        }
    }
    update_tip();
    return ToolResponse();
}
} // namespace horizon
