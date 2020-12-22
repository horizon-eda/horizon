#include "tool_map_package.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "imp/imp_interface.hpp"
#include "pool/part.hpp"
#include <iostream>

namespace horizon {

bool ToolMapPackage::can_begin()
{
    return doc.b;
}

ToolResponse ToolMapPackage::begin(const ToolArgs &args)
{
    Board *brd = doc.b->get_board();

    std::set<Component *> components_from_selection; // used for placing from schematic
    for (const auto &it : args.selection) {
        if (it.type == ObjectType::COMPONENT) {
            if (brd->block->components.count(it.uuid)) {
                auto &comp = brd->block->components.at(it.uuid);
                if (comp.part)
                    components_from_selection.insert(&comp);
            }
        }
    }

    std::set<Component *> components_placed;
    if (components_from_selection.size() > 0) {
        components_placed = components_from_selection;
    }
    else {
        for (auto &it : brd->block->components) {
            if (it.second.part)
                components_placed.insert(&it.second);
        }
    }

    for (auto &it : brd->packages) {
        components_placed.erase(it.second.component);
    }

    if (components_placed.size() == 0) {
        imp->tool_bar_flash("No packages left to place");
        return ToolResponse::end();
    }

    for (auto &it : components_placed) {
        components.push_back({it, false});
    }

    std::sort(components.begin(), components.end(),
              [](const auto &a, const auto &b) { return a.first->refdes < b.first->refdes; });

    UUID selected_component;
    if (components_from_selection.size() == 0) {
        if (auto r = imp->dialogs.map_package(components)) {
            selected_component = *r;
        }
        else {
            return ToolResponse::end();
        }

        auto x = std::find_if(components.begin(), components.end(),
                              [selected_component](const auto &a) { return a.first->uuid == selected_component; });
        assert(x != components.end());
        component_index = x - components.begin();
    }
    else {
        component_index = 0;
        selected_component = components.front().first->uuid;
    }

    Component *comp = &brd->block->components.at(selected_component);
    place_package(comp, args.coords);

    update_tooltip();

    imp->tool_bar_set_actions({
            {InToolActionID::LMB},
            {InToolActionID::RMB},
            {InToolActionID::ROTATE},
            {InToolActionID::MIRROR},
            {InToolActionID::EDIT, "select package"},
    });
    return ToolResponse();
}

void ToolMapPackage::place_package(Component *comp, const Coordi &c)
{
    Board *brd = doc.b->get_board();
    auto uu = UUID::random();
    pkg = &brd->packages.emplace(std::piecewise_construct, std::forward_as_tuple(uu), std::forward_as_tuple(uu, comp))
                   .first->second;
    pkg->placement.shift = c;
    pkg->flip = flipped;
    pkg->placement.set_angle(angle);
    pkg->update(*brd);

    nets.clear();
    for (const auto &it : pkg->package.pads) {
        if (it.second.net)
            nets.insert(it.second.net->uuid);
    }
    all_nets.insert(nets.begin(), nets.end());
    brd->update_airwires(true, nets);
    selection.clear();
    selection.emplace(uu, ObjectType::BOARD_PACKAGE);
    update_tooltip();
    move_init(c);
}

void ToolMapPackage::update_tooltip()
{
    if (pkg) {
        imp->tool_bar_set_tip("placing package " + pkg->component->refdes + " " + pkg->component->part->get_value());
    }
    else {
        imp->tool_bar_set_tip("");
    }
}

ToolResponse ToolMapPackage::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        move_do_cursor(args.coords);
        doc.b->get_board()->update_airwires(true, nets);
        return ToolResponse();
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB: {
            components.at(component_index).second = true;
            component_index++;
            while (component_index < components.size()) {
                if (components.at(component_index).second == false)
                    break;
                component_index++;
            }
            if (component_index == components.size()) {
                doc.b->get_board()->update_airwires(false, all_nets);
                return ToolResponse::commit();
            }
            Component *comp = components.at(component_index).first;
            place_package(comp, args.coords);
        } break;

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            if (pkg) {
                doc.b->get_board()->packages.erase(pkg->uuid);
            }
            doc.b->get_board()->update_airwires(false, all_nets);
            return ToolResponse::commit();

        case InToolActionID::EDIT:
            if (auto r = imp->dialogs.map_package(components)) {
                UUID selected_component = *r;
                doc.b->get_board()->packages.erase(pkg->uuid);

                auto x = std::find_if(components.begin(), components.end(), [selected_component](const auto &a) {
                    return a.first->uuid == selected_component;
                });
                assert(x != components.end());
                component_index = x - components.begin();

                Component *comp = components.at(component_index).first;
                place_package(comp, args.coords);
            }
            break;

        case InToolActionID::ROTATE:
        case InToolActionID::MIRROR:
            move_mirror_or_rotate(pkg->placement.shift, args.action == InToolActionID::ROTATE);
            flipped = pkg->flip;
            angle = pkg->placement.get_angle();
            break;

        default:;
        }
    }
    return ToolResponse();
}
} // namespace horizon
