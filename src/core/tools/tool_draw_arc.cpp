#include "tool_draw_arc.hpp"
#include "imp/imp_interface.hpp"
#include "document/idocument.hpp"
#include <iostream>
#include "common/arc.hpp"
#include <gdk/gdkkeysyms.h>
#include <sstream>

namespace horizon {

ToolDrawArc::ToolDrawArc(IDocument *c, ToolID tid) : ToolHelperLineWidthSetting(c, tid)
{
}

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
    std::stringstream ss;
    ss << "<b>LMB:</b>";
    if (state == DrawArcState::FROM) {
        ss << "place from junction";
    }
    else if (state == DrawArcState::TO) {
        ss << "place to junction";
    }
    else if (state == DrawArcState::CENTER) {
        ss << "place center junction";
    }
    ss << " <b>RMB:</b>cancel <b>e:</b>reverse arc direction <b>w:</b>line width";
    imp->tool_bar_set_tip(ss.str());
}

ToolResponse ToolDrawArc::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        temp_junc->position = args.coords;
    }
    else if (args.type == ToolEventType::CLICK) {
        if (args.button == 1) {
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
        }
        else if (args.button == 3) {
            return ToolResponse::revert();
        }
    }
    else if (args.type == ToolEventType::LAYER_CHANGE) {
        if (temp_arc)
            temp_arc->layer = args.work_layer;
    }
    else if (args.type == ToolEventType::KEY) {
        if (args.key == GDK_KEY_w) {
            ask_line_width();
        }
        else if (args.key == GDK_KEY_Escape) {
            return ToolResponse::revert();
        }
        else if (args.key == GDK_KEY_e) {
            if (temp_arc) {
                temp_arc->reverse();
            }
        }
    }
    update_tip();
    return ToolResponse();
}
} // namespace horizon
