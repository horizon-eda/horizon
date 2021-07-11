#include "tool_helper_merge.hpp"
#include "block/bus.hpp"
#include "block/net.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"
#include "imp/imp_interface.hpp"

namespace horizon {

int ToolHelperMerge::merge_nets(Net *net, Net *into)
{
    if (net->is_bussed || into->is_bussed) {
        if (net->is_power || into->is_power) {
            imp->tool_bar_flash("can't merge power and bussed net");
            return 1; // don't merge power and bus
        }
        else if (net->is_bussed && into->is_bussed) {
            imp->tool_bar_flash("can't merge bussed nets");
            return 1; // can't merge bussed nets
        }
        else if (net->is_port || into->is_port) {
            imp->tool_bar_flash("can't merge port and bussed net");
            return 1; // don't merge power and bus
        }
        else if (!net->is_bussed && into->is_bussed) {
            // merging non-bussed net into bussed net is fine
        }
        else if (net->is_bussed && !into->is_bussed) {
            std::swap(net, into);
        }
    }
    else if (net->is_power || into->is_power) {
        if (net->is_power && into->is_power) {
            imp->tool_bar_flash("can't merge power nets");
            return 1;
        }
        else if (net->is_port || into->is_port) {
            imp->tool_bar_flash("can't merge port and power net");
            return 1; // don't merge power and bus
        }
        else if (!net->is_power && into->is_power) {
            // merging non-power net into power net is fine
        }
        else if (net->is_power && !into->is_power) {
            std::swap(net, into);
        }
    }
    else if (net->is_port || into->is_port) {
        if (net->is_port && into->is_port) {
            imp->tool_bar_flash("can't merge port nets");
            return 1;
        }
        else if (!net->is_port && into->is_port) {
            // merging non-port net into port net is fine
        }
        else if (net->is_port && !into->is_port) {
            std::swap(net, into);
        }
    }
    else if (net->is_named() && into->is_named()) {
        int r = imp->dialogs.ask_net_merge(*net, *into);
        if (r == 1) {
            // nop, nets are already as we want them to
        }
        else if (r == 2) {
            std::swap(net, into);
        }
        else {
            // don't merge nets
            return 1;
        }
    }
    else if (net->is_named()) {
        std::swap(net, into);
    }

    imp->tool_bar_flash("merged net \"" + net->name + "\" into net \"" + into->name + "\"");
    doc.c->get_current_schematic()->block->merge_nets(net, into); // net will be erased
    doc.c->get_current_schematic()->expand_connectivity(true);    // be careful

    std::cout << "merging nets" << std::endl;
    return 0; // merged, 1: error
}

bool ToolHelperMerge::merge_bus_net(class Net *net, class Bus *bus, class Net *net_other, class Bus *bus_other)
{
    if (bus || bus_other) { // either is bus
        if (net || net_other) {
            imp->tool_bar_flash("can't connect bus and net");
            return false;
        }

        if (bus && bus_other) {     // both are bus
            if (bus != bus_other) { // not the same bus
                imp->tool_bar_flash("can't connect different buses");
                return false;
            }
            else {
                return true;
            }
        }
        if ((bus && !bus_other) || (!bus && bus_other)) { // one is bus
            return true;
        }
        assert(false); // dont go here
    }
    else if (net && net_other && net->uuid != net_other->uuid) { // both have nets, need to
                                                                 // merge nets
        if (merge_nets(net_other, net)) {                        // 1: fail
            imp->tool_bar_flash("net merge failed");
            return false;
        }
        else { // merge okay
            return true;
        }
    }
    else { // just connect it
        return true;
    }
    return false;
}

void ToolHelperMerge::merge_and_connect()
{
    if (!doc.c)
        return;
    auto &sheet = *doc.c->get_sheet();
    for (const auto &it : selection) {
        if (it.type == ObjectType::JUNCTION) {
            auto ju = &sheet.junctions.at(it.uuid);
            bool merged = false;
            for (auto &it_other : doc.c->get_sheet()->junctions) {
                auto ju_other = &it_other.second;
                if (!selection.count(SelectableRef(ju_other->uuid, ObjectType::JUNCTION))) { // not in selection
                    if (ju_other->position == ju->position) {                                // need to merge junctions
                        std::cout << "maybe merge junction" << std::endl;
                        Net *net = ju->net;
                        Bus *bus = ju->bus;
                        Net *net_other = ju_other->net;
                        Bus *bus_other = ju_other->bus;
                        auto do_merge = merge_bus_net(net, bus, net_other, bus_other);
                        if (do_merge) {
                            doc.c->get_sheet()->merge_junction(ju, ju_other);
                            merged = true;
                        }
                    }
                }
            }
            if (!merged) { // still not merged, maybe we can connect a symbol pin
                for (auto &[sym_uu, sym] : sheet.symbols) {
                    for (auto &[pin_uu, sym_pin] : sym.symbol.pins) {
                        if (sym.placement.transform(sym_pin.position) == ju->position) {
                            const auto path = UUIDPath<2>(sym.gate->uuid, pin_uu);
                            const bool pin_connected = sym.component->connections.count(path);
                            if (!pin_connected) {
                                if (ju->net) { // have net
                                    sym.component->connections.emplace(path, static_cast<Net *>(ju->net));
                                    sheet.replace_junction(ju, &sym, &sym_pin);
                                    merged = true;
                                }
                                else if (!ju->bus) {
                                    auto new_net = doc.c->get_current_block()->insert_net();
                                    sym.component->connections.emplace(path, new_net);
                                    sheet.replace_junction(ju, &sym, &sym_pin);
                                    doc.c->get_current_schematic()->expand(true);
                                    merged = true;
                                }
                            }
                        }
                    }
                }
            }
            if (!merged) { // still not merged, maybe we can connect a block symbol port
                for (auto &[sym_uu, sym] : sheet.block_symbols) {
                    for (auto &[port_uuz, sym_port] : sym.symbol.ports) {
                        if (sym.placement.transform(sym_port.position) == ju->position) {
                            const bool pin_connected = sym.block_instance->connections.count(sym_port.net);
                            if (!pin_connected) {
                                if (ju->net) { // have net
                                    sym.block_instance->connections.emplace(sym_port.net, static_cast<Net *>(ju->net));
                                    sheet.replace_junction(ju, &sym, &sym_port);
                                    merged = true;
                                }
                                else if (!ju->bus) {
                                    auto new_net = doc.c->get_current_block()->insert_net();
                                    sym.block_instance->connections.emplace(sym_port.net, new_net);
                                    sheet.replace_junction(ju, &sym, &sym_port);
                                    doc.c->get_current_schematic()->expand(true);
                                    merged = true;
                                }
                            }
                        }
                    }
                }
            }
        }
        else if (it.type == ObjectType::SCHEMATIC_SYMBOL) {
            auto &sym = doc.c->get_sheet()->symbols.at(it.uuid);
            doc.c->get_current_schematic()->autoconnect_symbol(doc.c->get_sheet(), &sym);
            if (sym.component->connections.size() == 0) {
                doc.c->get_current_schematic()->place_bipole_on_line(doc.c->get_sheet(), &sym);
            }
        }
        else if (it.type == ObjectType::SCHEMATIC_BLOCK_SYMBOL) {
            auto &sym = doc.c->get_sheet()->block_symbols.at(it.uuid);
            doc.c->get_current_schematic()->autoconnect_block_symbol(doc.c->get_sheet(), &sym);
        }
    }
}
} // namespace horizon
