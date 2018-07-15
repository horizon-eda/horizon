#include "tool_draw_line.hpp"
#include "imp/imp_interface.hpp"
#include "pool/part.hpp"
#include <algorithm>
#include "nlohmann/json.hpp"

namespace horizon {

ToolDrawLine::ToolDrawLine(Core *c, ToolID tid) : ToolHelperLineWidthSetting(c, tid)
{
}

bool ToolDrawLine::can_begin()
{
    return core.r->has_object_type(ObjectType::LINE);
}

void ToolDrawLine::apply_settings()
{
    if (temp_line)
        temp_line->width = settings.width;
}

ToolResponse ToolDrawLine::begin(const ToolArgs &args)
{
    std::cout << "tool draw line junction\n";

    temp_junc = core.r->insert_junction(UUID::random());
    junctions_created.insert(temp_junc);
    temp_junc->temp = true;
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
    s += " <b>w:</b>line width";
    imp->tool_bar_set_tip(s);
}

ToolResponse ToolDrawLine::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        temp_junc->position = args.coords;
        update_tip();
        return ToolResponse::fast();
    }
    else if (args.type == ToolEventType::CLICK) {
        if (args.button == 1) {
            if (args.target.type == ObjectType::JUNCTION) {
                if (temp_line != nullptr) {
                    temp_line->to = core.r->get_junction(args.target.path.at(0));
                }
                if (temp_line)
                    first_line = false;
                temp_line = core.r->insert_line(UUID::random());
                temp_line->from = core.r->get_junction(args.target.path.at(0));
            }
            else {
                Junction *last = temp_junc;
                temp_junc->temp = false;
                temp_junc = core.r->insert_junction(UUID::random());
                junctions_created.insert(temp_junc);
                temp_junc->temp = true;
                temp_junc->position = args.coords;

                if (temp_line)
                    first_line = false;
                temp_line = core.r->insert_line(UUID::random());
                temp_line->from = last;
            }
            temp_line->layer = args.work_layer;
            temp_line->width = settings.width;
            temp_line->to = temp_junc;
        }
        else if (args.button == 3) {
            if (temp_line) {
                if (first_line && junctions_created.count(temp_line->from))
                    core.r->delete_junction(temp_line->from->uuid);
                core.r->delete_line(temp_line->uuid);
                temp_line = nullptr;
                first_line = true;
            }
            else {
                core.r->delete_junction(temp_junc->uuid);
                temp_junc = nullptr;
                core.r->commit();
                return ToolResponse::end();
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
        else if (args.key == GDK_KEY_Escape) {
            if (temp_line) {
                if (first_line && junctions_created.count(temp_line->from))
                    core.r->delete_junction(temp_line->from->uuid);
                core.r->delete_line(temp_line->uuid);
                temp_line = nullptr;
            }
            core.r->delete_junction(temp_junc->uuid);
            temp_junc = nullptr;
            core.r->commit();
            return ToolResponse::end();
        }
    }
    update_tip();
    return ToolResponse();
}
} // namespace horizon
