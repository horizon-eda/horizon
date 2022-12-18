#include "tool_draw_line_net.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"
#include "imp/imp_interface.hpp"
#include "tool_helper_move.hpp"
#include <sstream>

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
    const auto uu = UUID::random();
    temp_junc_head = &doc.c->get_sheet()->junctions.emplace(uu, uu).first->second;
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
    doc.c->get_current_schematic()->expand_connectivity(); // gets rid of warning=s...
    const auto uu = UUID::random();
    temp_junc_head = &doc.c->get_sheet()->junctions.emplace(uu, uu).first->second;
    temp_junc_head->position = c;
    temp_junc_mid = nullptr;
    temp_line_head = nullptr;
    temp_line_mid = nullptr;
    component_floating = nullptr;
    block_instance_floating = nullptr;
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

SchematicJunction *ToolDrawLineNet::make_temp_junc(const Coordi &c)
{
    const auto uu = UUID::random();
    SchematicJunction *j = &doc.c->get_sheet()->junctions.emplace(uu, uu).first->second;
    j->position = c;
    return j;
}

void ToolDrawLineNet::update_tip()
{
    std::vector<ActionLabelInfo> actions;
    actions.reserve(12);
    actions.emplace_back(InToolActionID::LMB, "connect");
    actions.emplace_back(InToolActionID::RMB);
    actions.emplace_back(InToolActionID::PLACE_JUNCTION);
    actions.emplace_back(InToolActionID::COMMIT, "finish");
    actions.emplace_back(InToolActionID::POSTURE);
    actions.emplace_back(InToolActionID::ARBITRARY_ANGLE_MODE);

    std::stringstream ss;
    if (net_label) {
        actions.emplace_back(InToolActionID::NET_LABEL_SIZE_INC, InToolActionID::NET_LABEL_SIZE_DEC, "label size");
        actions.emplace_back(InToolActionID::ENTER_SIZE, "enter label size");
        actions.emplace_back(InToolActionID::ROTATE, "rotate label");
    }
    if (temp_line_head) {
        if (temp_line_head->net) {
            actions.emplace_back(InToolActionID::TOGGLE_NET_LABEL);
            if (temp_line_head->net->name.size()) {
                ss << "drawing net \"" << temp_line_head->net->name << "\"";
            }
            else {
                ss << "drawing unnamed net";
            }
        }
        else if (temp_line_head->bus) {
            ss << "drawing bus \"" << temp_line_head->bus->name << "\"";
        }
        else {
            ss << "drawing no net";
        }
    }
    else {
        ss << "select starting point";
    }
    imp->tool_bar_set_tip(ss.str());
    imp->tool_bar_set_actions(actions);
}

void ToolDrawLineNet::apply_settings()
{
    if (net_label) {
        net_label->size = settings.net_label_size;
    }
}

LineNet *ToolDrawLineNet::insert_line_net()
{
    auto uu = UUID::random();
    return &doc.c->get_sheet()
                    ->net_lines.emplace(std::piecewise_construct, std::forward_as_tuple(uu), std::forward_as_tuple(uu))
                    .first->second;
}

ToolResponse ToolDrawLineNet::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        move_temp_junc(args.coords);
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB: {
            SchematicJunction *ju = nullptr;
            SchematicSymbol *sym = nullptr;
            uuid_ptr<SymbolPin> pin = nullptr;
            SchematicBlockSymbol *block_sym = nullptr;
            uuid_ptr<BlockSymbolPort> port = nullptr;
            BusRipper *rip = nullptr;
            Net *net = nullptr;
            Bus *bus = nullptr;
            if (!temp_line_head) { // no line yet, first click
                if (args.target.type == ObjectType::JUNCTION) {
                    ju = &doc.c->get_sheet()->junctions.at(args.target.path.at(0));
                    net = ju->net;
                    bus = ju->bus;
                    if (ju->connected_power_symbols.size() == 1) {
                        const auto &pwrsym = doc.c->get_sheet()->power_symbols.at(ju->connected_power_symbols.front());
                        switch (pwrsym.orientation) {
                        case Orientation::UP:
                        case Orientation::DOWN:
                            bend_mode = BendMode::YX;
                            break;

                        case Orientation::LEFT:
                        case Orientation::RIGHT:
                            bend_mode = BendMode::XY;
                            break;
                        }
                    }
                }
                else if (args.target.type == ObjectType::SYMBOL_PIN) {
                    sym = &doc.c->get_sheet()->symbols.at(args.target.path.at(0));
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
                        net = doc.c->get_current_schematic()->block->insert_net();
                        sym->component->connections.emplace(connpath, net);
                        component_floating = sym->component;
                        connpath_floating = connpath;
                    }
                }
                else if (args.target.type == ObjectType::BLOCK_SYMBOL_PORT) {
                    block_sym = &doc.c->get_sheet()->block_symbols.at(args.target.path.at(0));
                    port = &block_sym->symbol.ports.at(args.target.path.at(1));
                    port_start = port;
                    const UUID net_port = port->net;

                    const auto orientation = port->get_orientation_for_placement(block_sym->placement);
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

                    if (block_sym->block_instance->connections.count(net_port)) { // port is connected
                        Connection &conn = block_sym->block_instance->connections.at(net_port);
                        if (conn.net == nullptr) {
                            imp->tool_bar_flash("can't connect to NC port");
                            return ToolResponse();
                        }
                        net = conn.net;
                    }
                    else { // pin needs net
                        net = doc.c->get_current_schematic()->block->insert_net();
                        block_sym->block_instance->connections.emplace(net_port, net);
                        block_instance_floating = block_sym->block_instance;
                        net_port_floating = net_port;
                    }
                }
                else if (args.target.type == ObjectType::BUS_RIPPER) {
                    rip = &doc.c->get_sheet()->bus_rippers.at(args.target.path.at(0));
                    net = rip->bus_member->net;
                }
                else { // unknown or no target
                    ju = make_temp_junc(args.coords);
                    for (auto &[uu, it] : doc.c->get_sheet()->net_lines) {
                        if (it.coord_on_line(temp_junc_head->position)) {
                            if (&it != temp_line_head && &it != temp_line_mid) {
                                imp->tool_bar_flash("split net line");
                                auto li = doc.c->get_sheet()->split_line_net(&it, ju);
                                net = li->net;
                                bus = li->bus;
                                ju->net = li->net;
                                ju->bus = li->bus;
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
                if (args.target.type == ObjectType::JUNCTION) {
                    ju = &doc.c->get_sheet()->junctions.at(args.target.path.at(0));
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
                    sym = &doc.c->get_sheet()->symbols.at(args.target.path.at(0));
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
                            net = doc.c->get_current_schematic()->block->insert_net();
                        }
                        sym->component->connections.emplace(connpath, net);
                    }
                    temp_line_head->to.connect(sym, pin);
                    doc.r->delete_junction(temp_junc_head->uuid);
                    restart(args.coords);
                    return ToolResponse();
                }
                else if (args.target.type == ObjectType::BLOCK_SYMBOL_PORT) {
                    block_sym = &doc.c->get_sheet()->block_symbols.at(args.target.path.at(0));
                    port = &block_sym->symbol.ports.at(args.target.path.at(1));
                    if (port == port_start) // do noting
                        return ToolResponse();
                    const UUID net_port = port->net;
                    if (temp_line_head->bus) { // can't connect bus to port
                        return ToolResponse();
                    }
                    if (block_sym->block_instance->connections.count(net_port)) { // port is connected
                        Connection &conn = block_sym->block_instance->connections.at(net_port);
                        if (conn.net == nullptr) {
                            imp->tool_bar_flash("can't connect to NC pin");
                            return ToolResponse();
                        }
                        if (temp_line_head->net && (temp_line_head->net->uuid != conn.net->uuid)) { // has net
                            if (merge_nets(conn.net, temp_line_head->net)) {
                                return ToolResponse();
                            }
                            port = &block_sym->symbol.ports.at(args.target.path.at(1)); // update port after merge,
                                                                                        // since merge replaces ports
                        }
                    }
                    else { // port needs net
                        if (temp_junc_head->net) {
                            net = temp_junc_head->net;
                        }
                        else {
                            net = doc.c->get_current_schematic()->block->insert_net();
                        }
                        block_sym->block_instance->connections.emplace(net_port, net);
                    }
                    temp_line_head->to.connect(block_sym, port);
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
                    for (auto &[uu, it] : doc.c->get_sheet()->net_lines) {
                        if (it.coord_on_line(temp_junc_head->position)) {
                            if (&it != temp_line_head && &it != temp_line_mid) {
                                imp->tool_bar_flash("split net line");
                                net = it.net;
                                bus = it.bus;

                                auto do_merge = merge_bus_net(temp_line_head->net, temp_line_head->bus, it.net, it.bus);
                                if (do_merge) {
                                    doc.c->get_sheet()->split_line_net(&it, ju);
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


            temp_line_mid = insert_line_net();
            temp_line_mid->net = net;
            temp_line_mid->bus = bus;
            if (ju) {
                temp_line_mid->from.connect(ju);
            }
            else if (sym) {
                temp_line_mid->from.connect(sym, pin);
            }
            else if (block_sym) {
                temp_line_mid->from.connect(block_sym, port);
            }
            else if (rip) {
                temp_line_mid->from.connect(rip);
            }
            else {
                assert(false);
            }
            temp_line_mid->to.connect(temp_junc_mid);

            temp_line_head = insert_line_net();
            temp_line_head->net = net;
            temp_line_head->bus = bus;
            temp_line_head->from.connect(temp_junc_mid);
            temp_line_head->to.connect(temp_junc_head);
        } break;

        case InToolActionID::RMB:
            if (temp_line_head) {
                doc.c->get_sheet()->net_lines.erase(temp_line_head->uuid);
                doc.c->get_sheet()->net_lines.erase(temp_line_mid->uuid);
                doc.r->delete_junction(temp_junc_mid->uuid);
                cleanup_floating();
                restart(args.coords);
                return ToolResponse();
            }
            else {
                doc.r->delete_junction(temp_junc_head->uuid);
                return ToolResponse::commit();
            }
            break;

        case InToolActionID::CANCEL:
            if (temp_line_head) {
                doc.c->get_sheet()->net_lines.erase(temp_line_head->uuid);
                doc.c->get_sheet()->net_lines.erase(temp_line_mid->uuid);
                doc.r->delete_junction(temp_junc_mid->uuid);
                cleanup_floating();
            }
            doc.r->delete_junction(temp_junc_head->uuid);
            return ToolResponse::commit();

        case InToolActionID::PLACE_JUNCTION: {
            SchematicJunction *ju = temp_junc_head;
            temp_junc_mid = make_temp_junc(args.coords);
            temp_junc_mid->net = ju->net;
            temp_junc_mid->bus = ju->bus;

            temp_junc_head = make_temp_junc(args.coords);
            temp_junc_head->net = ju->net;
            temp_junc_head->bus = ju->bus;

            temp_line_mid = insert_line_net();
            temp_line_mid->net = ju->net;
            temp_line_mid->bus = ju->bus;
            temp_line_mid->from.connect(ju);
            temp_line_mid->to.connect(temp_junc_mid);

            temp_line_head = insert_line_net();
            temp_line_head->net = ju->net;
            temp_line_head->bus = ju->bus;
            temp_line_head->from.connect(temp_junc_mid);
            temp_line_head->to.connect(temp_junc_head);

            set_snap_filter();
        } break;

        case InToolActionID::COMMIT:
            return ToolResponse::commit();

        case InToolActionID::POSTURE:
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
            break;

        case InToolActionID::ARBITRARY_ANGLE_MODE:
            bend_mode = BendMode::ARB;
            move_temp_junc(args.coords);
            break;

        case InToolActionID::TOGGLE_NET_LABEL:
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
            break;

        case InToolActionID::NET_LABEL_SIZE_INC:
            if (net_label)
                step_net_label_size(true);
            break;

        case InToolActionID::NET_LABEL_SIZE_DEC:
            if (net_label)
                step_net_label_size(false);
            break;

        case InToolActionID::ENTER_SIZE:
            if (net_label)
                ask_net_label_size();
            break;

        case InToolActionID::ROTATE:
            if (net_label)
                net_label->orientation = ToolHelperMove::transform_orientation(net_label->orientation, true);
            break;

        default:;
        }
    }
    update_tip();
    return ToolResponse();
}

void ToolDrawLineNet::cleanup_floating()
{
    if (component_floating)
        component_floating->connections.erase(connpath_floating);
    if (block_instance_floating)
        block_instance_floating->connections.erase(net_port_floating);
}

} // namespace horizon
