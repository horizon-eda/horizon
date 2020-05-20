#include "tool_place_bus_ripper.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"
#include "imp/imp_interface.hpp"
#include "tool_helper_move.hpp"
#include "util/util.hpp"
#include <iostream>
#include <gdk/gdkkeysyms.h>

namespace horizon {

ToolPlaceBusRipper::ToolPlaceBusRipper(IDocument *c, ToolID tid)
    : ToolBase(c, tid), ToolPlaceJunction(c, tid), ToolHelperMove(c, tid)
{
}

bool ToolPlaceBusRipper::can_begin()
{
    return doc.c;
}

bool ToolPlaceBusRipper::begin_attached()
{
    std::cout << "tool place bus ripper\n";

    bool r;
    UUID bus_uuid;
    std::tie(r, bus_uuid) = imp->dialogs.select_bus(doc.c->get_schematic()->block);
    if (!r) {
        return false;
    }
    bus = &doc.c->get_schematic()->block->buses.at(bus_uuid);

    for (auto &it : bus->members) {
        bus_members.push_back(&it.second);
    }
    if (bus_members.size() == 0)
        return false;
    std::sort(bus_members.begin(), bus_members.end(),
              [](auto a, auto b) { return strcmp_natural(a->name, b->name) < 0; });
    imp->tool_bar_set_tip(
            "<b>LMB:</b>place bus ripper <b>RMB:</b>delete current ripper and "
            "finish <b>space:</b>select member <b>r:</b>rotate <b>e:</b>mirror");
    return true;
}

void ToolPlaceBusRipper::create_attached()
{
    Orientation orientation = Orientation::UP;
    if (ri) {
        orientation = ri->orientation;
    }
    auto uu = UUID::random();
    ri = &doc.c->get_sheet()->bus_rippers.emplace(uu, uu).first->second;
    ri->bus = bus;
    ri->bus_member = bus_members.at(bus_member_current);
    ri->junction = temp;
    ri->orientation = orientation;
    bus_member_current++;
    bus_member_current %= bus_members.size();
}

void ToolPlaceBusRipper::delete_attached()
{
    if (ri) {
        doc.c->get_sheet()->bus_rippers.erase(ri->uuid);
        temp->bus = nullptr;
        ri = nullptr;
    }
}

bool ToolPlaceBusRipper::check_line(LineNet *li)
{
    if (li->net)
        return false;
    return li->bus == bus;
}

bool ToolPlaceBusRipper::update_attached(const ToolArgs &args)
{
    if (args.type == ToolEventType::CLICK) {
        if (args.button == 1) {
            if (args.target.type == ObjectType::JUNCTION) {
                Junction *j = doc.r->get_junction(args.target.path.at(0));
                if (j->bus != bus) {
                    imp->tool_bar_flash("junction connected to wrong bus");
                    return true;
                }
                ri->junction = j;
                create_attached();
            }
            else {
                for (auto it : doc.c->get_net_lines()) {
                    if (it->coord_on_line(temp->position)) {
                        std::cout << "on line" << std::endl;
                        if (it->bus == bus) {
                            doc.c->get_sheet()->split_line_net(it, temp);
                            junctions_placed.push_front(temp);
                            create_junction(args.coords);
                            create_attached();
                            return true;
                        }
                        else {
                            imp->tool_bar_flash("line connected to wrong bus");
                            return true;
                        }
                    }
                }
                imp->tool_bar_flash("can't place bus ripper nowhere");
            }
            return true;
        }
    }
    else if (args.type == ToolEventType::KEY) {
        if (args.key == GDK_KEY_space) {
            bool r;
            UUID bus_member_uuid;

            std::tie(r, bus_member_uuid) = imp->dialogs.select_bus_member(doc.c->get_schematic()->block, bus->uuid);
            if (!r)
                return true;
            Bus::Member *bus_member = &bus->members.at(bus_member_uuid);

            auto p = std::find(bus_members.begin(), bus_members.end(), bus_member);
            bus_member_current = p - bus_members.begin();
            delete_attached();
            create_attached();

            return true;
        }
        else if (args.key == GDK_KEY_e) {
            ri->mirror();
        }
        else if (args.key == GDK_KEY_r) {
            ri->orientation = transform_orientation(ri->orientation, true);
        }
    }

    return false;
}
} // namespace horizon
