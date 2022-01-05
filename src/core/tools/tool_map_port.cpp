#include "tool_map_port.hpp"
#include "document/idocument_block_symbol.hpp"
#include "block_symbol/block_symbol.hpp"
#include "block/block.hpp"
#include "imp/imp_interface.hpp"
#include "tool_helper_move.hpp"
#include "util/util.hpp"
#include "util/geom_util.hpp"
#include "canvas/canvas_gl.hpp"

namespace horizon {

bool ToolMapPort::can_begin()
{
    return doc.o;
}

void ToolMapPort::create_port(const UUID &net)
{
    auto orientation = Orientation::RIGHT;
    if (port) {
        orientation = port->orientation;
    }
    port_last2 = port_last;
    port_last = port;

    const auto uu = BlockSymbolPort::get_uuid_for_net(net);
    port = &doc.o->get_block_symbol()
                    .ports.emplace(std::piecewise_construct, std::forward_as_tuple(uu), std::forward_as_tuple(uu))
                    .first->second;
    port->name = doc.o->get_block_symbol().block->get_net_name(net);
    port->net = net;
    port->direction = doc.o->get_block_symbol().block->nets.at(net).port_direction;
    port->orientation = orientation;
    update_annotation();
}

ToolResponse ToolMapPort::begin(const ToolArgs &args)
{
    bool from_sidebar = false;
    for (const auto &it : args.selection) {
        if (it.type == ObjectType::BLOCK_SYMBOL_PORT) {
            if (doc.o->get_block_symbol().get_port_for_net(it.uuid) == nullptr) { // unplaced port
                port_nets.emplace_back(&doc.o->get_block_symbol().block->nets.at(it.uuid), false);
                from_sidebar = true;
            }
        }
    }
    if (port_nets.size() == 0) {
        for (const auto &[uu, net] : doc.o->get_block_symbol().block->nets) {
            if (net.is_port)
                port_nets.emplace_back(&net, false);
        }
    }
    std::sort(port_nets.begin(), port_nets.end(),
              [](const auto &a, const auto &b) { return strcmp_natural(a.first->name, b.first->name) < 0; });

    for (auto &it : port_nets) {
        if (doc.o->get_block_symbol().ports.count(it.first->uuid)) {
            it.second = true;
        }
    }
    auto net_ports_unplaced =
            std::count_if(port_nets.begin(), port_nets.end(), [](const auto &a) { return a.second == false; });
    if (net_ports_unplaced == 0) {
        imp->tool_bar_flash("No ports left to place");
        return ToolResponse::end();
    }

    UUID selected_port;
    if (net_ports_unplaced > 1 && from_sidebar == false) {
        if (auto r = map_port_dialog()) {
            selected_port = *r;
        }
        else {
            return ToolResponse::end();
        }
    }
    else {
        selected_port = std::find_if(port_nets.begin(), port_nets.end(), [](const auto &a) {
                            return a.second == false;
                        })->first->uuid;
    }


    auto x = std::find_if(port_nets.begin(), port_nets.end(),
                          [selected_port](const auto &a) { return a.first->uuid == selected_port; });
    assert(x != port_nets.end());
    port_index = x - port_nets.begin();

    annotation = imp->get_canvas()->create_annotation();
    annotation->set_visible(true);
    annotation->set_display(LayerDisplay(true, LayerDisplay::Mode::OUTLINE));

    create_port(selected_port);
    port->position = args.coords;
    update_tip();

    selection.clear();

    return ToolResponse();
}

bool ToolMapPort::can_autoplace() const
{
    if (port_last && port_last2)
        return port_last2->orientation == port_last->orientation;
    else
        return false;
}

void ToolMapPort::update_annotation()
{
    if (!annotation)
        return;
    annotation->clear();
    if (can_autoplace()) {
        auto shift = port_last->position - port_last2->position;
        Coordf center = port_last->position + shift;
        float size = 0.25_mm;
        std::vector<Coordf> pts = {center + Coordf(-size, size), center + Coordf(size, size),
                                   center + Coordf(size, -size), center + Coordf(-size, -size)};
        for (size_t i = 0; i < pts.size(); i++) {
            const auto &p0 = pts.at(i);
            const auto &p2 = pts.at((i + 1) % pts.size());
            annotation->draw_line(p0, p2, ColorP::PIN_HIDDEN, 0);
        }

        Placement pl;
        pl.set_angle(orientation_to_angle(port_last->orientation));
        Coordf p = pl.transform(Coordf(port_last->length, 0));
        annotation->draw_line(center, center - p, ColorP::PIN_HIDDEN, 0);
    }
}

void ToolMapPort::update_tip()
{

    std::vector<ActionLabelInfo> actions;
    actions.reserve(8);
    actions.emplace_back(InToolActionID::LMB);
    actions.emplace_back(InToolActionID::RMB);
    actions.emplace_back(InToolActionID::ROTATE);
    actions.emplace_back(InToolActionID::MIRROR);
    actions.emplace_back(InToolActionID::EDIT, "select port");
    if (can_autoplace()) {
        actions.emplace_back(InToolActionID::AUTOPLACE_NEXT_PIN);
        actions.emplace_back(InToolActionID::AUTOPLACE_ALL_PINS);
    }

    imp->tool_bar_set_actions(actions);
}

ToolResponse ToolMapPort::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        port->position = args.coords;
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB:
            port_nets.at(port_index).second = true;
            port_index++;
            while (port_index < port_nets.size()) {
                if (port_nets.at(port_index).second == false)
                    break;
                port_index++;
            }
            if (port_index == port_nets.size()) {
                return ToolResponse::commit();
            }
            create_port(port_nets.at(port_index).first->uuid);
            port->position = args.coords;
            break;

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            if (port) {
                doc.o->get_block_symbol().ports.erase(port->uuid);
            }
            return ToolResponse::commit();

        case InToolActionID::AUTOPLACE_ALL_PINS:
        case InToolActionID::AUTOPLACE_NEXT_PIN:
            if (can_autoplace()) {
                auto shift = port_last->position - port_last2->position;
                while (1) {
                    port->position = port_last->position + shift;

                    port_nets.at(port_index).second = true;
                    port_index++;
                    while (port_index < port_nets.size()) {
                        if (port_nets.at(port_index).second == false)
                            break;
                        port_index++;
                    }
                    if (port_index == port_nets.size()) {
                        return ToolResponse::commit();
                    }
                    create_port(port_nets.at(port_index).first->uuid);
                    port->position = args.coords;

                    if (args.action == InToolActionID::AUTOPLACE_NEXT_PIN)
                        break;
                }
            }
            break;

        case InToolActionID::EDIT: {
            if (auto r = map_port_dialog()) {
                UUID selected_port = *r;
                doc.o->get_block_symbol().ports.erase(port->uuid);

                auto x = std::find_if(port_nets.begin(), port_nets.end(),
                                      [selected_port](const auto &a) { return a.first->uuid == selected_port; });
                assert(x != port_nets.end());
                port_index = x - port_nets.begin();

                auto p1 = port_last;
                auto p2 = port_last2;
                create_port(selected_port);
                port->position = args.coords;
                port_last2 = p2;
                port_last = p1;
            }
        } break;

        case InToolActionID::ROTATE:
        case InToolActionID::MIRROR:
            port->orientation =
                    ToolHelperMove::transform_orientation(port->orientation, args.action == InToolActionID::ROTATE);
            break;

        default:;
        }
    }
    update_tip();
    return ToolResponse();
}

std::optional<UUID> ToolMapPort::map_port_dialog()
{
    std::map<UUIDPath<2>, std::string> items_for_dialog;
    for (const auto &[it, mapped] : port_nets) {
        if (!mapped)
            items_for_dialog.emplace(std::piecewise_construct, std::forward_as_tuple(it->uuid),
                                     std::forward_as_tuple(it->name));
    }
    if (auto r = imp->dialogs.map_uuid_path("Map Port", items_for_dialog))
        return r->at(0);
    else
        return {};
}

ToolMapPort::~ToolMapPort()
{
    if (annotation) {
        imp->get_canvas()->remove_annotation(annotation);
        annotation = nullptr;
    }
}

} // namespace horizon
