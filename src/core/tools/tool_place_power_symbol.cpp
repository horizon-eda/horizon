#include "tool_place_power_symbol.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"
#include "imp/imp_interface.hpp"
#include "tool_helper_move.hpp"
#include <iostream>
#include <gdk/gdkkeysyms.h>

namespace horizon {

ToolPlacePowerSymbol::ToolPlacePowerSymbol(IDocument *c, ToolID tid) : ToolBase(c, tid), ToolPlaceJunction(c, tid)
{
}

bool ToolPlacePowerSymbol::can_begin()
{
    return doc.c;
}

bool ToolPlacePowerSymbol::begin_attached()
{
    bool r;
    UUID net_uuid;
    std::tie(r, net_uuid) = imp->dialogs.select_net(doc.c->get_schematic()->block, true);
    if (!r) {
        return false;
    }
    net = &doc.c->get_schematic()->block->nets.at(net_uuid);
    imp->tool_bar_set_tip(
            "<b>LMB:</b>place power symbol <b>RMB:</b>delete current power "
            "symbol and finish <b>e:</b>mirror  <b>r:</b>rotate");
    return true;
}

void ToolPlacePowerSymbol::create_attached()
{
    auto old_sym = sym;
    auto uu = UUID::random();
    sym = &doc.c->get_sheet()->power_symbols.emplace(uu, uu).first->second;
    sym->net = net;
    sym->junction = temp;
    if (old_sym) {
        sym->mirror = old_sym->mirror;
        sym->orientation = old_sym->orientation;
    }
    temp->net = net;
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
        doc.c->get_schematic()->block->merge_nets(other, net);
        imp->tool_bar_flash("merged net \"" + other->name + "\" into power net\"" + net->name + "\"");
        doc.c->get_schematic()->expand(true);
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
    if (args.type == ToolEventType::CLICK) {
        if (args.button == 1) {
            if (args.target.type == ObjectType::JUNCTION) {
                Junction *j = doc.r->get_junction(args.target.path.at(0));
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
                create_attached();
                return true;
            }
            if (args.target.type == ObjectType::SYMBOL_PIN) {
                SchematicSymbol *schsym = doc.c->get_schematic_symbol(args.target.path.at(0));
                SymbolPin *pin = &schsym->symbol.pins.at(args.target.path.at(1));
                UUIDPath<2> connpath(schsym->gate->uuid, args.target.path.at(1));
                if (schsym->component->connections.count(connpath) == 0) {
                    // sympin not connected
                    auto uu = UUID::random();
                    auto *line = &doc.c->get_sheet()->net_lines.emplace(uu, uu).first->second;
                    line->net = net;
                    line->from.connect(schsym, pin);
                    line->to.connect(temp);
                    schsym->component->connections.emplace(UUIDPath<2>(schsym->gate->uuid, pin->uuid), net);

                    temp->position.y -= 1.25_mm;
                    create_junction(args.coords);
                    create_attached();
                }

                return true;
            }
        }
    }
    else if (args.type == ToolEventType::KEY) {
        if (args.key == GDK_KEY_e) {
            sym->mirrorx();
        }
        else if (args.key == GDK_KEY_r) {
            sym->orientation = ToolHelperMove::transform_orientation(sym->orientation, true, false);
        }
    }

    return false;
}
} // namespace horizon
