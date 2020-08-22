#include "tool_place_bus_label.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"
#include "imp/imp_interface.hpp"
#include "tool_helper_move.hpp"
#include <iostream>

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
    if (auto r = imp->dialogs.select_bus(*doc.c->get_schematic()->block)) {
        imp->tool_bar_set_actions({
                {InToolActionID::LMB},
                {InToolActionID::RMB},
                {InToolActionID::ROTATE},
                {InToolActionID::MIRROR},
        });
        bus = &doc.c->get_schematic()->block->buses.at(*r);
        return true;
    }
    else {
        return false;
    }
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
    if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB:
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
            break;

        case InToolActionID::ROTATE:
        case InToolActionID::MIRROR:
            la->orientation =
                    ToolHelperMove::transform_orientation(la->orientation, args.action == InToolActionID::ROTATE);
            last_orientation = la->orientation;
            return true;

        default:;
        }
    }

    return false;
}
} // namespace horizon
