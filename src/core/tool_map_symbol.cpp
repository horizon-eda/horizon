#include "tool_map_symbol.hpp"
#include "core_schematic.hpp"
#include "imp/imp_interface.hpp"
#include "pool/entity.hpp"
#include <iostream>

namespace horizon {

ToolMapSymbol::ToolMapSymbol(Core *c, ToolID tid)
    : ToolBase(c, tid), ToolHelperMove(c, tid), ToolHelperMapSymbol(c, tid)
{
}

bool ToolMapSymbol::can_begin()
{
    return core.c;
}

ToolResponse ToolMapSymbol::begin(const ToolArgs &args)
{
    std::cout << "tool map sym\n";
    Schematic *sch = core.c->get_schematic();
    // collect gates
    std::set<UUIDPath<2>> gates;

    // find all gates
    for (const auto &it_component : sch->block->components) {
        for (const auto &it_gate : it_component.second.entity->gates) {
            gates.emplace(it_component.first, it_gate.first);
        }
    }

    // remove placed gates
    for (auto &it_sheet : sch->sheets) {
        Sheet &sheet = it_sheet.second;

        for (const auto &it_sym : sheet.symbols) {
            const auto &sym = it_sym.second;
            if (gates.count({sym.component->uuid, sym.gate->uuid})) {
                gates.erase({sym.component->uuid, sym.gate->uuid});
            }
        }
    }

    for (const auto &it : gates) {
        Component *comp = &sch->block->components.at(it.at(0));
        const Gate *gate = &comp->entity->gates.at(it.at(1));
        gates_out.emplace(std::make_pair(it, comp->refdes + gate->suffix));
    }

    if (gates_out.size() == 0) {
        imp->tool_bar_flash("all symbols placed");
        return ToolResponse::end();
    }

    UUIDPath<2> selected_gate;
    bool r;
    if (gates_out.size() > 1) {
        std::tie(r, selected_gate) = imp->dialogs.map_symbol(gates_out);
        if (!r) {
            return ToolResponse::end();
        }
    }
    else {
        selected_gate = gates_out.begin()->first;
    }

    Component *comp = &sch->block->components.at(selected_gate.at(0));
    const Gate *gate = &comp->entity->gates.at(selected_gate.at(1));

    sym_current = map_symbol(comp, gate);
    if (!sym_current) {
        core.r->revert();
        return ToolResponse::end();
    }
    sym_current->placement.shift = args.coords;

    core.c->selection.clear();
    core.c->selection.emplace(sym_current->uuid, ObjectType::SCHEMATIC_SYMBOL);

    move_init(args.coords);

    return ToolResponse();
}
void ToolMapSymbol::update_tip()
{
    imp->tool_bar_set_tip(
            "<b>LMB:</b>place <b>RMB:</b>cancel <b>r:</b>rotate "
            "<b>e:</b>mirror");
}

ToolResponse ToolMapSymbol::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        move_do_cursor(args.coords);
    }
    else if (args.type == ToolEventType::CLICK) {
        if (args.button == 1) {

            gates_out.erase(UUIDPath<2>(sym_current->component->uuid, sym_current->gate->uuid));

            UUIDPath<2> selected_gate;
            bool r;
            if (gates_out.size() == 0) {
                core.r->commit();
                return ToolResponse::end();
            }
            else if (gates_out.size() == 1) {
                selected_gate = gates_out.begin()->first;
            }
            else {
                std::tie(r, selected_gate) = imp->dialogs.map_symbol(gates_out);
                if (!r) {
                    core.r->commit();
                    return ToolResponse::end();
                }
            }
            Schematic *sch = core.c->get_schematic();

            Component *comp = &sch->block->components.at(selected_gate.at(0));
            const Gate *gate = &comp->entity->gates.at(selected_gate.at(1));

            sym_current = map_symbol(comp, gate);
            if (!sym_current) {
                core.r->revert();
                return ToolResponse::end();
            }
            sym_current->placement.shift = args.coords;

            core.c->selection.clear();
            core.c->selection.emplace(sym_current->uuid, ObjectType::SCHEMATIC_SYMBOL);
        }
        else {
            core.c->delete_schematic_symbol(sym_current->uuid);
            sym_current = nullptr;
            core.r->commit();
            return ToolResponse::end();
        }
    }
    else if (args.type == ToolEventType::KEY) {
        if (args.key == GDK_KEY_Escape) {
            core.r->revert();
            return ToolResponse::end();
        }
        else if (args.key == GDK_KEY_r || args.key == GDK_KEY_e) {
            bool rotate = args.key == GDK_KEY_r;
            move_mirror_or_rotate(sym_current->placement.shift, rotate);
        }
    }
    update_tip();
    return ToolResponse();
}
} // namespace horizon
