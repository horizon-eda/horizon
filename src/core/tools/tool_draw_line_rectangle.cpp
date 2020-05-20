#include "tool_draw_line_rectangle.hpp"
#include "common/polygon.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>
#include <sstream>
#include "common/line.hpp"
#include "document/idocument.hpp"
#include <gdk/gdkkeysyms.h>

namespace horizon {

ToolDrawLineRectangle::ToolDrawLineRectangle(IDocument *c, ToolID tid) : ToolHelperLineWidthSetting(c, tid)
{
}

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
    std::stringstream ss;
    ss << "<b>LMB:</b>";
    if (mode == Mode::CENTER) {
        if (step == 0) {
            ss << "place center";
        }
        else {
            ss << "place corner";
        }
    }
    else {
        if (step == 0) {
            ss << "place first corner";
        }
        else {
            ss << "place second corner";
        }
    }
    ss << " <b>RMB:</b>cancel";
    ss << " <b>c:</b>switch mode";
    ss << "  <b>w:</b>line width";

    ss << " <i>";
    if (mode == Mode::CENTER) {
        ss << "from center";
    }
    else {
        ss << "corners";
    }
    ss << " </i>";

    imp->tool_bar_set_tip(ss.str());
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
    else if (args.type == ToolEventType::CLICK) {
        if (args.button == 1) {
            if (step == 0) {
                step = 1;
            }
            else {
                for (int i = 0; i < 4; i++) {
                }
                return ToolResponse::commit();
            }
        }
        else if (args.button == 3) {
            return ToolResponse::revert();
        }
    }
    else if (args.type == ToolEventType::KEY) {
        if (args.key == GDK_KEY_c) {
            mode = mode == Mode::CENTER ? Mode::CORNER : Mode::CENTER;
            update_junctions();
        }
        else if (args.key == GDK_KEY_w) {
            ask_line_width();
        }
        else if (args.key == GDK_KEY_Escape) {
            return ToolResponse::revert();
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
