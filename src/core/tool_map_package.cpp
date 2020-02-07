#include "tool_map_package.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "imp/imp_interface.hpp"
#include "pool/part.hpp"
#include <iostream>
#include <gdk/gdkkeysyms.h>

namespace horizon {
ToolMapPackage::ToolMapPackage(IDocument *c, ToolID tid) : ToolBase(c, tid), ToolHelperMove(c, tid)
{
}

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
        bool r;
        std::tie(r, selected_component) = imp->dialogs.map_package(components);
        if (!r) {
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
    brd->packages_expand = {uu};
    brd->expand_flags = static_cast<Board::ExpandFlags>(Board::EXPAND_PACKAGES);
    brd->expand(true);
    nets.clear();
    for (const auto &it : pkg->package.pads) {
        if (it.second.net)
            nets.insert(it.second.net->uuid);
    }
    selection.clear();
    selection.emplace(uu, ObjectType::BOARD_PACKAGE);
    update_tooltip();
    move_init(c);
}

void ToolMapPackage::update_tooltip()
{
    std::string text =
            "<b>LMB:</b>place package <b>RMB:</b>delete current package and "
            "finish <b>r:</b>rotate <b>e:</b>mirror <b>Space</b>:select "
            "package";
    if (pkg) {
        text += " <i>placing package " + pkg->component->refdes + " " + pkg->component->part->get_value() + "</i>";
    }
    imp->tool_bar_set_tip(text);
}

ToolResponse ToolMapPackage::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        move_do_cursor(args.coords);
        doc.b->get_board()->update_airwires(true, nets);
        return ToolResponse::fast();
    }
    else if (args.type == ToolEventType::CLICK) {
        if (args.button == 1) {
            components.at(component_index).second = true;
            component_index++;
            while (component_index < components.size()) {
                if (components.at(component_index).second == false)
                    break;
                component_index++;
            }
            if (component_index == components.size()) {
                return ToolResponse::commit();
            }
            Component *comp = components.at(component_index).first;
            place_package(comp, args.coords);
        }
        else if (args.button == 3) {
            if (pkg) {
                doc.b->get_board()->packages.erase(pkg->uuid);
            }
            return ToolResponse::commit();
        }
    }
    else if (args.type == ToolEventType::KEY) {
        if (args.key == GDK_KEY_space) {
            bool r;
            UUID selected_component;
            std::tie(r, selected_component) = imp->dialogs.map_package(components);
            if (r) {
                doc.b->get_board()->packages.erase(pkg->uuid);

                auto x = std::find_if(components.begin(), components.end(), [selected_component](const auto &a) {
                    return a.first->uuid == selected_component;
                });
                assert(x != components.end());
                component_index = x - components.begin();

                Component *comp = components.at(component_index).first;
                place_package(comp, args.coords);
            }
        }
        else if (args.key == GDK_KEY_r || args.key == GDK_KEY_e) {
            bool rotate = args.key == GDK_KEY_r;
            move_mirror_or_rotate(pkg->placement.shift, rotate);
            flipped = pkg->flip;
            angle = pkg->placement.get_angle();
        }
        else if (args.key == GDK_KEY_Escape) {
            return ToolResponse::revert();
        }
    }
    return ToolResponse();
}
} // namespace horizon
