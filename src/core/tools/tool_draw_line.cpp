#include "tool_draw_line.hpp"
#include "imp/imp_interface.hpp"
#include "pool/part.hpp"
#include <algorithm>
#include "nlohmann/json.hpp"
#include "document/idocument.hpp"
#include <gdk/gdkkeysyms.h>

namespace horizon {

ToolDrawLine::ToolDrawLine(IDocument *c, ToolID tid) : ToolHelperLineWidthSetting(c, tid)
{
}

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
    std::cout << "tool draw line junction\n";

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
    std::string s("<b>LMB:</b>place junction/connect <b>RMB:</b>");
    if (temp_line) {
        s += "finish current segment";
    }
    else {
        s += "end tool";
    }
    s += " <b>w:</b>line width ";
    s += "<b>/:</b>restrict <i>";
    s += restrict_mode_to_string();
    s += "</i>";
    imp->tool_bar_set_tip(s);
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
        return ToolResponse::fast();
    }
    else if (args.type == ToolEventType::CLICK) {
        if (args.button == 1) {
            if (args.target.type == ObjectType::JUNCTION && restrict_mode == RestrictMode::ARB) {
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
        }
        else if (args.button == 3) {
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
        }
    }
    else if (args.type == ToolEventType::LAYER_CHANGE) {
        if (temp_line)
            temp_line->layer = args.work_layer;
    }
    else if (args.type == ToolEventType::KEY) {
        if (args.key == GDK_KEY_w) {
            ask_line_width();
        }
        else if (args.key == GDK_KEY_slash) {
            cycle_restrict_mode();
            do_move(args.coords);
        }
        else if (args.key == GDK_KEY_Escape) {
            if (temp_line) {
                if (first_line && junctions_created.count(temp_line->from))
                    doc.r->delete_junction(temp_line->from->uuid);
                doc.r->delete_line(temp_line->uuid);
                temp_line = nullptr;
            }
            doc.r->delete_junction(temp_junc->uuid);
            temp_junc = nullptr;
            return ToolResponse::commit();
        }
    }
    update_tip();
    return ToolResponse();
}
} // namespace horizon
