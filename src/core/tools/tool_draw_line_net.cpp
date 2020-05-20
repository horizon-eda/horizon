#include "tool_draw_line_net.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"
#include "imp/imp_interface.hpp"
#include "tool_helper_move.hpp"
#include <sstream>
#include <gdk/gdkkeysyms.h>

namespace horizon {

ToolDrawLineNet::ToolDrawLineNet(IDocument *c, ToolID tid)
    : ToolBase(c, tid), ToolHelperMerge(c, tid), ToolHelperDrawNetSetting(c, tid)
{
}

bool ToolDrawLineNet::can_begin()
{
    return doc.c;
}

ToolResponse ToolDrawLineNet::begin(const ToolArgs &args)
{
    std::cout << "tool draw net line junction\n";

    temp_junc_head = doc.c->insert_junction(UUID::random());
    imp->set_snap_filter({{ObjectType::JUNCTION, temp_junc_head->uuid}});
    temp_junc_head->position = args.coords;
    selection.clear();

    update_tip();
    return ToolResponse();
}

void ToolDrawLineNet::set_snap_filter()
{
    std::set<SnapFilter> sf;
    if (temp_junc_head)
        sf.emplace(ObjectType::JUNCTION, temp_junc_head->uuid);
    if (temp_junc_mid)
        sf.emplace(ObjectType::JUNCTION, temp_junc_mid->uuid);
    imp->set_snap_filter(sf);
}

void ToolDrawLineNet::restart(const Coordi &c)
{
    doc.c->get_schematic()->expand(); // gets rid of warnings...
    temp_junc_head = doc.c->insert_junction(UUID::random());
    temp_junc_head->position = c;
    temp_junc_mid = nullptr;
    temp_line_head = nullptr;
    temp_line_mid = nullptr;
    component_floating = nullptr;
    net_label = nullptr;
    set_snap_filter();
    update_tip();
}

void ToolDrawLineNet::move_temp_junc(const Coordi &c)
{
    temp_junc_head->position = c;
    if (temp_line_head) {
        auto tail_pos = temp_line_mid->from.get_position();
        auto head_pos = temp_junc_head->position;

        if (bend_mode == BendMode::XY) {
            temp_junc_mid->position = {head_pos.x, tail_pos.y};
        }
        else if (bend_mode == BendMode::YX) {
            temp_junc_mid->position = {tail_pos.x, head_pos.y};
        }
        else if (bend_mode == BendMode::ARB) {
            temp_junc_mid->position = tail_pos;
        }
    }
}

Junction *ToolDrawLineNet::make_temp_junc(const Coordi &c)
{
    Junction *j = doc.r->insert_junction(UUID::random());
    j->position = c;
    return j;
}

void ToolDrawLineNet::update_tip()
{
    std::stringstream ss;
    ss << "<b>LMB:</b>place junction/connect <b>RMB:</b>finish and delete last "
          "segment <b>space:</b>place junction <b>⏎:</b>finish <b>/:</b>line "
          "posture <b>a:</b>arbitrary angle ";
    if (net_label) {
        ss << "<b>+-:</b>change label size <b>s:</b>set label size ";
    }
    if (temp_line_head) {
        if (temp_line_head->net) {
            ss << "<b>b:</b>toggle label <b>r:</b>rotate label";
            ss << " <i>";
            if (temp_line_head->net->name.size()) {
                ss << "drawing net \"" << temp_line_head->net->name << "\"";
            }
            else {
                ss << "drawing unnamed net";
            }
        }
        else if (temp_line_head->bus) {
            ss << " <i>";
            ss << "drawing bus \"" << temp_line_head->bus->name << "\"";
        }
        else {
            ss << " <i>";
            ss << "drawing no net";
        }
    }
    else {
        ss << " <i>";
        ss << "select starting point";
    }
    ss << "</i>";
    imp->tool_bar_set_tip(ss.str());
}

void ToolDrawLineNet::apply_settings()
{
    if (net_label) {
        net_label->size = settings.net_label_size;
    }
}

ToolResponse ToolDrawLineNet::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        move_temp_junc(args.coords);
    }
    else if (args.type == ToolEventType::CLICK) {
        if (args.button == 1) {
            Junction *ju = nullptr;
            SchematicSymbol *sym = nullptr;
            uuid_ptr<SymbolPin> pin = nullptr;
            BusRipper *rip = nullptr;
            Net *net = nullptr;
            Bus *bus = nullptr;
            if (!temp_line_head) { // no line yet, first click
                if (args.target.type == ObjectType::JUNCTION) {
                    ju = doc.r->get_junction(args.target.path.at(0));
                    net = ju->net;
                    bus = ju->bus;
                }
                else if (args.target.type == ObjectType::SYMBOL_PIN) {
                    sym = doc.c->get_schematic_symbol(args.target.path.at(0));
                    pin = &sym->symbol.pins.at(args.target.path.at(1));
                    pin_start = pin;
                    UUIDPath<2> connpath(sym->gate->uuid, args.target.path.at(1));

                    auto orientation = pin->get_orientation_for_placement(sym->placement);
                    switch (orientation) {
                    case Orientation::UP:
                    case Orientation::DOWN:
                        bend_mode = BendMode::YX;
                        break;

                    case Orientation::LEFT:
                    case Orientation::RIGHT:
                        bend_mode = BendMode::XY;
                        break;
                    }

                    if (sym->component->connections.count(connpath)) { // pin is connected
                        Connection &conn = sym->component->connections.at(connpath);
                        if (conn.net == nullptr) {
                            imp->tool_bar_flash("can't connect to NC pin");
                            return ToolResponse();
                        }
                        net = conn.net;
                    }
                    else { // pin needs net
                        net = doc.c->get_schematic()->block->insert_net();
                        sym->component->connections.emplace(connpath, net);
                        component_floating = sym->component;
                        connpath_floating = connpath;
                    }
                }
                else if (args.target.type == ObjectType::BUS_RIPPER) {
                    rip = &doc.c->get_sheet()->bus_rippers.at(args.target.path.at(0));
                    net = rip->bus_member->net;
                }
                else { // unknown or no target
                    ju = make_temp_junc(args.coords);
                    for (auto it : doc.c->get_net_lines()) {
                        if (it->coord_on_line(temp_junc_head->position)) {
                            if (it != temp_line_head && it != temp_line_mid) {
                                imp->tool_bar_flash("split net line");
                                auto li = doc.c->get_sheet()->split_line_net(it, ju);
                                net = li->net;
                                bus = li->bus;
                                ju->net = li->net;
                                ju->bus = li->bus;
                                ju->connection_count = 3;
                                break;
                            }
                        }
                    }
                }

                temp_junc_mid = make_temp_junc(args.coords);
                temp_junc_mid->net = net;
                temp_junc_mid->bus = bus;
                temp_junc_head->net = net;
                temp_junc_head->bus = bus;
                set_snap_filter();
            }
            else { // already have net line
                component_floating = nullptr;
                if (args.target.type == ObjectType::JUNCTION) {
                    ju = doc.r->get_junction(args.target.path.at(0));
                    auto do_merge = merge_bus_net(temp_line_head->net, temp_line_head->bus, ju->net, ju->bus);
                    if (do_merge) {
                        temp_line_head->to.connect(ju);
                        doc.r->delete_junction(temp_junc_head->uuid);
                        restart(args.coords);
                        return ToolResponse();
                    }
                    else {
                        return ToolResponse();
                    }
                }
                else if (args.target.type == ObjectType::SYMBOL_PIN) {
                    sym = doc.c->get_schematic_symbol(args.target.path.at(0));
                    pin = &sym->symbol.pins.at(args.target.path.at(1));
                    if (pin == pin_start) // do noting
                        return ToolResponse();
                    UUIDPath<2> connpath(sym->gate->uuid, args.target.path.at(1));
                    if (temp_line_head->bus) { // can't connect bus to pin
                        return ToolResponse();
                    }
                    if (sym->component->connections.count(connpath)) { // pin is connected
                        Connection &conn = sym->component->connections.at(connpath);
                        if (conn.net == nullptr) {
                            imp->tool_bar_flash("can't connect to NC pin");
                            return ToolResponse();
                        }
                        if (temp_line_head->net && (temp_line_head->net->uuid != conn.net->uuid)) { // has net
                            if (merge_nets(conn.net, temp_line_head->net)) {
                                return ToolResponse();
                            }
                            pin = &sym->symbol.pins.at(args.target.path.at(1)); // update pin after merge, since merge
                                                                                // replaces pins
                        }
                    }
                    else { // pin needs net
                        if (temp_junc_head->net) {
                            net = temp_junc_head->net;
                        }
                        else {
                            net = doc.c->get_schematic()->block->insert_net();
                        }
                        sym->component->connections.emplace(connpath, net);
                    }
                    temp_line_head->to.connect(sym, pin);
                    doc.r->delete_junction(temp_junc_head->uuid);
                    restart(args.coords);
                    return ToolResponse();
                }
                else if (args.target.type == ObjectType::BUS_RIPPER) {
                    rip = &doc.c->get_sheet()->bus_rippers.at(args.target.path.at(0));
                    net = rip->bus_member->net;
                    if (temp_line_head->bus) { // can't connect bus to bus
                                               // ripper
                        return ToolResponse();
                    }
                    if (temp_line_head->net && (temp_line_head->net->uuid != net->uuid)) { // has net
                        if (merge_nets(net, temp_line_head->net)) {
                            return ToolResponse();
                        }
                    }

                    temp_line_head->to.connect(rip);
                    doc.r->delete_junction(temp_junc_head->uuid);
                    restart(args.coords);
                    return ToolResponse();
                }
                else { // unknown or no target
                    if (temp_line_mid->from.get_position() == args.coords) {
                        restart(args.coords);
                        return ToolResponse();
                    }
                    if (net_label) {
                        restart(args.coords);
                        return ToolResponse();
                    }

                    ju = temp_junc_head;
                    for (auto it : doc.c->get_net_lines()) {
                        if (it->coord_on_line(temp_junc_head->position)) {
                            if (it != temp_line_head && it != temp_line_mid) {
                                imp->tool_bar_flash("split net line");
                                net = it->net;
                                bus = it->bus;

                                auto do_merge =
                                        merge_bus_net(temp_line_head->net, temp_line_head->bus, it->net, it->bus);
                                if (do_merge) {
                                    doc.c->get_sheet()->split_line_net(it, ju);
                                    restart(args.coords);
                                    return ToolResponse();
                                }
                                else {
                                    return ToolResponse(); // bus-net
                                                           // illegal
                                }
                                break;
                            }
                        }
                    }
                    net = ju->net;
                    bus = ju->bus;
                    temp_junc_mid = make_temp_junc(args.coords);
                    temp_junc_mid->net = net;
                    temp_junc_mid->bus = bus;

                    temp_junc_head = make_temp_junc(args.coords);
                    temp_junc_head->net = net;
                    temp_junc_head->bus = bus;
                    set_snap_filter();
                }
            }
            temp_line_mid = doc.c->insert_line_net(UUID::random());
            temp_line_mid->net = net;
            temp_line_mid->bus = bus;
            if (ju) {
                temp_line_mid->from.connect(ju);
            }
            else if (sym) {
                temp_line_mid->from.connect(sym, pin);
            }
            else if (rip) {
                temp_line_mid->from.connect(rip);
            }
            else {
                assert(false);
            }
            temp_line_mid->to.connect(temp_junc_mid);

            temp_line_head = doc.c->insert_line_net(UUID::random());
            temp_line_head->net = net;
            temp_line_head->bus = bus;
            temp_line_head->from.connect(temp_junc_mid);
            temp_line_head->to.connect(temp_junc_head);
        }
        else if (args.button == 3) {
            if (temp_line_head) {
                doc.c->delete_line_net(temp_line_head->uuid);
                doc.c->delete_line_net(temp_line_mid->uuid);
                doc.r->delete_junction(temp_junc_mid->uuid);
                if (component_floating)
                    component_floating->connections.erase(connpath_floating);
                restart(args.coords);
                return ToolResponse();
            }
            else {
                doc.r->delete_junction(temp_junc_head->uuid);
                return ToolResponse::commit();
            }
        }
    }
    else if (args.type == ToolEventType::KEY) {
        if (args.key == GDK_KEY_Escape) {
            if (temp_line_head) {
                doc.c->delete_line_net(temp_line_head->uuid);
                doc.c->delete_line_net(temp_line_mid->uuid);
                doc.r->delete_junction(temp_junc_mid->uuid);
                if (component_floating)
                    component_floating->connections.erase(connpath_floating);
            }
            doc.r->delete_junction(temp_junc_head->uuid);
            return ToolResponse::commit();
        }
        else if (args.key == GDK_KEY_space) {
            Junction *ju = temp_junc_head;
            temp_junc_mid = make_temp_junc(args.coords);
            temp_junc_mid->net = ju->net;
            temp_junc_mid->bus = ju->bus;

            temp_junc_head = make_temp_junc(args.coords);
            temp_junc_head->net = ju->net;
            temp_junc_head->bus = ju->bus;

            temp_line_mid = doc.c->insert_line_net(UUID::random());
            temp_line_mid->net = ju->net;
            temp_line_mid->bus = ju->bus;
            temp_line_mid->from.connect(ju);
            temp_line_mid->to.connect(temp_junc_mid);

            temp_line_head = doc.c->insert_line_net(UUID::random());
            temp_line_head->net = ju->net;
            temp_line_head->bus = ju->bus;
            temp_line_head->from.connect(temp_junc_mid);
            temp_line_head->to.connect(temp_junc_head);

            set_snap_filter();
        }
        else if (args.key == GDK_KEY_Return) {
            return ToolResponse::commit();
        }
        else if (args.key == GDK_KEY_slash) {
            switch (bend_mode) {
            case BendMode::XY:
                bend_mode = BendMode::YX;
                break;
            case BendMode::YX:
                bend_mode = BendMode::XY;
                break;
            default:
                bend_mode = BendMode::XY;
            }
            move_temp_junc(args.coords);
        }
        else if (args.key == GDK_KEY_b) {
            if (temp_junc_head && !temp_junc_head->bus) {
                if (!net_label) {
                    auto uu = UUID::random();
                    net_label = &doc.c->get_sheet()->net_labels.emplace(uu, uu).first->second;
                    net_label->junction = temp_junc_head;
                    net_label->size = settings.net_label_size;
                }
                else {
                    doc.c->get_sheet()->net_labels.erase(net_label->uuid);
                    net_label = nullptr;
                }
            }
        }
        else if (args.key == GDK_KEY_plus || args.key == GDK_KEY_equal) {
            if (net_label)
                step_net_label_size(true);
        }
        else if (args.key == GDK_KEY_minus) {
            if (net_label)
                step_net_label_size(false);
        }
        else if (args.key == GDK_KEY_s) {
            if (net_label)
                ask_net_label_size();
        }
        else if (args.key == GDK_KEY_r) {
            if (net_label)
                net_label->orientation =
                        ToolHelperMove::transform_orientation(net_label->orientation, args.key == GDK_KEY_r);
        }
        else if (args.key == GDK_KEY_a) {
            bend_mode = BendMode::ARB;
            move_temp_junc(args.coords);
        }
    }
    update_tip();
    return ToolResponse();
}
} // namespace horizon
