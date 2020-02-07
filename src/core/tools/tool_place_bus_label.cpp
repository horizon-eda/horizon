#include "tool_place_bus_label.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"
#include "imp/imp_interface.hpp"
#include "tool_helper_move.hpp"
#include <iostream>
#include <gdk/gdkkeysyms.h>

namespace horizon {

ToolPlaceBusLabel::ToolPlaceBusLabel(IDocument *c, ToolID tid) : ToolBase(c, tid), ToolPlaceJunction(c, tid)
{
}

bool ToolPlaceBusLabel::can_begin()
{
    return doc.c;
}

bool ToolPlaceBusLabel::begin_attached()
{
    bool r;
    UUID net_uuid;
    std::tie(r, net_uuid) = imp->dialogs.select_bus(doc.c->get_schematic()->block);
    if (!r) {
        return false;
    }
    bus = &doc.c->get_schematic()->block->buses.at(net_uuid);
    imp->tool_bar_set_tip(
            "<b>LMB:</b>place bus label <b>RMB:</b>delete current label and "
            "finish <b>r:</b>rotate <b>e:</b>mirror");
    return true;
}

void ToolPlaceBusLabel::create_attached()
{
    auto uu = UUID::random();
    la = &doc.c->get_sheet()->bus_labels.emplace(uu, uu).first->second;
    la->bus = bus;
    la->orientation = last_orientation;
    la->junction = temp;
    temp->bus = bus;
}

void ToolPlaceBusLabel::delete_attached()
{
    if (la) {
        doc.c->get_sheet()->bus_labels.erase(la->uuid);
        temp->net = nullptr;
    }
}

bool ToolPlaceBusLabel::check_line(LineNet *li)
{
    if (li->net)
        return false;
    if (!li->bus)
        return true;
    if (li->bus != bus)
        return false;
    return true;
}

bool ToolPlaceBusLabel::update_attached(const ToolArgs &args)
{
    if (args.type == ToolEventType::CLICK) {
        if (args.button == 1) {
            if (args.target.type == ObjectType::JUNCTION) {
                Junction *j = doc.r->get_junction(args.target.path.at(0));
                if (j->net)
                    return true;
                if (j->bus && j->bus != bus)
                    return true;
                la->junction = j;
                j->bus = bus;
                create_attached();
                return true;
            }
        }
    }
    if (args.key == GDK_KEY_r || args.key == GDK_KEY_e) {
        la->orientation = ToolHelperMove::transform_orientation(la->orientation, args.key == GDK_KEY_r);
        last_orientation = la->orientation;
        return true;
    }

    return false;
}
} // namespace horizon
