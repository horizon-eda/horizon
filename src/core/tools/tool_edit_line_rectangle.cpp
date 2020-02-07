#include "tool_edit_line_rectangle.hpp"
#include "common/polygon.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>
#include "document/idocument.hpp"
#include "common/line.hpp"
#include <gdk/gdkkeysyms.h>

namespace horizon {

ToolEditLineRectangle::ToolEditLineRectangle(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolEditLineRectangle::can_begin()
{
    if (!doc.r->has_object_type(ObjectType::LINE))
        return false;
    for (const auto &it : selection) {
        if (it.type == ObjectType::JUNCTION || it.type == ObjectType::LINE) {
            return true;
        }
    }
    return false;
}

void ToolEditLineRectangle::update_junctions(const Coordi &c)
{
    auto p0t = c;
    auto p1t = c * -1;
    Coordi p0 = Coordi::min(p0t, p1t);
    Coordi p1 = Coordi::max(p0t, p1t);
    junctions[0]->position = p0;
    junctions[1]->position = {p0.x, p1.y};
    junctions[2]->position = p1;
    junctions[3]->position = {p1.x, p0.y};
}

ToolResponse ToolEditLineRectangle::begin(const ToolArgs &args)
{
    std::cout << "tool edit line\n";

    Junction *start = nullptr;
    for (const auto &it : args.selection) {
        if (it.type == ObjectType::JUNCTION) {
            start = doc.r->get_junction(it.uuid);
            break;
        }
        else if (it.type == ObjectType::LINE) {
            start = doc.r->get_line(it.uuid)->from;
            break;
        }
    }
    if (!start) {
        imp->tool_bar_flash("no start junction found");
        return ToolResponse::revert();
    }
    std::set<Line *> lines_found;
    std::set<Junction *> junctions_found;
    junctions_found.insert(start);
    bool found = true;
    while (found) {
        found = false;
        for (auto li : doc.r->get_lines()) {
            if (junctions_found.count(li->from)) {
                if (junctions_found.insert(li->to).second)
                    found = true;
            }
            if (junctions_found.count(li->to)) {
                if (junctions_found.insert(li->from).second)
                    found = true;
            }
        }
    }
    if (junctions_found.size() != 4) {
        imp->tool_bar_flash("didn't find 4 junctions");
        return ToolResponse::revert();
    }

    // figure out correct order
    junctions[0] = *junctions_found.begin();
    junctions_found.erase(junctions[0]);

    for (int i = 1; i < 4; i++) {
        auto ju_prev = junctions[i - 1];
        junctions[i] = nullptr;
        for (auto li : doc.r->get_lines()) {
            if (li->from == ju_prev && junctions_found.count(li->to)) {
                junctions[i] = li->to;
                break;
            }
            else if (li->to == ju_prev && junctions_found.count(li->from)) {
                junctions[i] = li->from;
                break;
            }
        }
        if (junctions[i]) {
            junctions_found.erase(junctions[i]);
        }
        else {
            imp->tool_bar_flash("topology error");
            return ToolResponse::revert();
        }
    }

    selection.clear();
    for (auto ju : junctions) {
        selection.emplace(ju->uuid, ObjectType::JUNCTION);
    }
    update_junctions(args.coords);
    imp->tool_bar_set_tip("<b>LMB:</b>finish <b>RMB:</b>cancel");
    return ToolResponse();
}

ToolResponse ToolEditLineRectangle::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        update_junctions(args.coords);
    }
    else if (args.type == ToolEventType::CLICK) {
        if (args.button == 1) {
            return ToolResponse::commit();
        }
        else if (args.button == 3) {
            return ToolResponse::revert();
        }
    }
    else if (args.type == ToolEventType::KEY) {
        if (args.key == GDK_KEY_Escape) {
            return ToolResponse::revert();
        }
    }
    return ToolResponse();
}
} // namespace horizon
