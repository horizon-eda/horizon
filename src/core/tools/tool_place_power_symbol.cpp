#include "tool_place_power_symbol.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"
#include "imp/imp_interface.hpp"
#include "tool_helper_move.hpp"
#include <iostream>

namespace horizon {

bool ToolPlacePowerSymbol::can_begin()
{
    return doc.c;
}

bool ToolPlacePowerSymbol::begin_attached()
{
    if (auto r = imp->dialogs.select_net(*doc.c->get_current_schematic()->block, true)) {
        net = &doc.c->get_current_schematic()->block->nets.at(*r);
        imp->tool_bar_set_actions({
                {InToolActionID::LMB},
                {InToolActionID::RMB},
                {InToolActionID::ROTATE},
                {InToolActionID::MIRROR},
        });
        return true;
    }
    else {
        return false;
    }
}

void ToolPlacePowerSymbol::create_attached()
{
    auto old_sym = sym;
    auto uu = UUID::random();
    sym = &doc.c->get_sheet()->power_symbols.emplace(uu, uu).first->second;
    sym->net = net;
    sym->junction = temp;

    switch (net->power_symbol_style) {
    case Net::PowerSymbolStyle::ANTENNA:
    case Net::PowerSymbolStyle::DOT:
        sym->orientation = Orientation::UP;
        break;

    case Net::PowerSymbolStyle::GND:
    case Net::PowerSymbolStyle::EARTH:
        sym->orientation = Orientation::DOWN;
        break;
    }

    if (old_sym) {
        sym->mirror = old_sym->mirror;
        sym->orientation = old_sym->orientation;
    }
    temp->net = net;
}

bool ToolPlacePowerSymbol::junction_placed()
{
    if (ToolPlaceJunctionSchematic::junction_placed())
        return true;

    doc.c->get_current_schematic()->expand_connectivity(true);
    return false;
}


void ToolPlacePowerSymbol::delete_attached()
{
    if (sym) {
        doc.c->get_sheet()->power_symbols.erase(sym->uuid);
        temp->net = nullptr;
    }
}

bool ToolPlacePowerSymbol::do_merge(Net *other)
{
    if (!other)
        return true;
    if (other->is_bussed)
        return false; // can't merge with bussed net
    if (other->is_power && other != net) {
        // junction is connected to other power net, can't merge
        return false;
    }
    else if (!other->is_power && other != net) {
        doc.c->get_current_schematic()->block->merge_nets(other, net);
        imp->tool_bar_flash("merged net \"" + other->name + "\" into power net\"" + net->name + "\"");
        return true;
    }
    else if (other->is_power && other == net) {
        return true;
    }
    return false;
}

bool ToolPlacePowerSymbol::check_line(LineNet *li)
{
    if (li->bus)
        return false;

    return do_merge(li->net);
}

bool ToolPlacePowerSymbol::update_attached(const ToolArgs &args)
{
    if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB:
            if (args.target.type == ObjectType::JUNCTION) {
                SchematicJunction *j = &doc.c->get_sheet()->junctions.at(args.target.path.at(0));
                if (j->bus)
                    return true;
                bool merged = do_merge(j->net);
                if (!merged) {
                    return true;
                }
                if (!j->net) {
                    j->net = net;
                }
                sym->junction = j;
                doc.c->get_current_schematic()->expand_connectivity(true);
                create_attached();
                return true;
            }
            else if (args.target.type == ObjectType::SYMBOL_PIN) {
                auto &schsym = doc.c->get_sheet()->symbols.at(args.target.path.at(0));
                auto &pin = schsym.symbol.pins.at(args.target.path.at(1));
                UUIDPath<2> connpath(schsym.gate->uuid, args.target.path.at(1));
                if (schsym.component->connections.count(connpath) == 0) {
                    // sympin not connected
                    auto uu = UUID::random();
                    auto *line = &doc.c->get_sheet()->net_lines.emplace(uu, uu).first->second;
                    line->net = net;
                    line->from.connect(&schsym, &pin);
                    line->to.connect(temp);
                    schsym.component->connections.emplace(UUIDPath<2>(schsym.gate->uuid, pin.uuid), net);

                    Coordi offset;
                    switch (sym->orientation) {
                    case Orientation::DOWN:
                        offset.y = -1;
                        break;

                    case Orientation::UP:
                        offset.y = 1;
                        break;

                    case Orientation::RIGHT:
                        offset.x = 1;
                        break;

                    case Orientation::LEFT:
                        offset.x = -1;
                        break;
                    }

                    temp->position += offset * 1.25_mm;
                    create_junction(args.coords);
                    create_attached();
                }
                return true;
            }
            return false;

        case InToolActionID::ROTATE:
            sym->orientation = ToolHelperMove::transform_orientation(sym->orientation, true, false);
            return true;

        case InToolActionID::MIRROR:
            sym->mirrorx();
            return true;

        default:;
        }
    }
    return false;
}
} // namespace horizon
