#include "tool_place_bus_ripper.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"
#include "imp/imp_interface.hpp"
#include "tool_helper_move.hpp"
#include "util/util.hpp"
#include "util/selection_util.hpp"
#include <iostream>

namespace horizon {

ToolPlaceBusRipper::ToolPlaceBusRipper(IDocument *c, ToolID tid)
    : ToolBase(c, tid), ToolPlaceJunctionSchematic(c, tid), ToolHelperMove(c, tid)
{
}

Bus *ToolPlaceBusRipper::get_bus()
{
    auto sr = sel_find_exactly_one(selection, ObjectType::LINE_NET);
    if (!sr)
        return nullptr;
    auto &net_line = doc.c->get_sheet()->net_lines.at(sr->uuid);
    return net_line.bus;
}

bool ToolPlaceBusRipper::can_begin()
{
    return doc.c && get_bus();
}

bool ToolPlaceBusRipper::begin_attached()
{
    bus = get_bus();
    if (!bus)
        return false;
    imp->tool_bar_set_actions({
            {InToolActionID::LMB},
            {InToolActionID::RMB},
            {InToolActionID::ROTATE},
            {InToolActionID::MIRROR},
            {InToolActionID::EDIT, "select member"},
    });
    std::map<const Bus::Member *, size_t> bus_rippers;

    for (auto &it : bus->members) {
        bus_members.push_back(&it.second);
        bus_rippers.emplace(&it.second, 0);
    }
    if (bus_members.size() == 0)
        return false;
    for (const auto &[sheet_uu, sheet] : doc.c->get_current_schematic()->sheets) {
        for (const auto &[uu, rip] : sheet.bus_rippers) {
            if (bus_rippers.count(rip.bus_member.ptr) && rip.bus == bus) {
                bus_rippers.at(rip.bus_member.ptr)++;
            }
        }
    }
    std::sort(bus_members.begin(), bus_members.end(),
              [](auto a, auto b) { return strcmp_natural(a->name, b->name) < 0; });
    std::stable_sort(bus_members.begin(), bus_members.end(),
                     [&bus_rippers](auto a, auto b) { return bus_rippers.at(a) < bus_rippers.at(b); });
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
    if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB:
            if (args.target.type == ObjectType::JUNCTION) {
                SchematicJunction *j = &doc.c->get_sheet()->junctions.at(args.target.path.at(0));
                if (j->bus != bus) {
                    imp->tool_bar_flash("junction connected to wrong bus");
                    return true;
                }
                ri->junction = j;
                create_attached();
            }
            else {
                for (auto &[uu, it] : doc.c->get_sheet()->net_lines) {
                    if (it.coord_on_line(temp->position)) {
                        std::cout << "on line" << std::endl;
                        if (it.bus == bus) {
                            doc.c->get_sheet()->split_line_net(&it, temp);
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

        case InToolActionID::EDIT:
            if (auto r = imp->dialogs.select_bus_member(*doc.c->get_current_schematic()->block, bus->uuid)) {
                Bus::Member *bus_member = &bus->members.at(*r);

                auto p = std::find(bus_members.begin(), bus_members.end(), bus_member);
                bus_member_current = p - bus_members.begin();
                delete_attached();
                create_attached();
            }
            return true;

        case InToolActionID::ROTATE:
            ri->orientation = transform_orientation(ri->orientation, true);
            break;

        case InToolActionID::MIRROR:
            ri->mirror();
            break;

        default:;
        }
    }
    return false;
}
} // namespace horizon
