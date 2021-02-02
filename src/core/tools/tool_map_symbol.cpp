#include "tool_map_symbol.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"
#include "imp/imp_interface.hpp"
#include "pool/entity.hpp"
#include <iostream>

namespace horizon {

ToolMapSymbol::ToolMapSymbol(IDocument *c, ToolID tid)
    : ToolBase(c, tid), ToolHelperMapSymbol(c, tid), ToolHelperMove(c, tid)
{
}

bool ToolMapSymbol::can_begin()
{
    return doc.c;
}

ToolResponse ToolMapSymbol::begin(const ToolArgs &args)
{
    std::cout << "tool map sym\n";
    const auto sch = doc.c->get_schematic();

    if (auto data = dynamic_cast<const ToolDataMapSymbol *>(args.data.get())) {
        std::copy(data->gates.begin(), data->gates.end(), std::back_inserter(gates_from_data));
    }
    UUIDPath<2> selected_gate;
    if (gates_from_data.size() == 0) {

        gates_out = sch->get_unplaced_gates();

        if (gates_out.size() == 0) {
            imp->tool_bar_flash("all symbols placed");
            return ToolResponse::end();
        }

        if (gates_out.size() > 1) {
            if (auto r = imp->dialogs.map_symbol(gates_out)) {
                selected_gate = *r;
            }
            else {
                return ToolResponse::end();
            }
        }
        else {
            selected_gate = gates_out.begin()->first;
        }
    }
    else {
        data_mode = true;
        selected_gate = gates_from_data.front();
    }

    Component *comp = &sch->block->components.at(selected_gate.at(0));
    const Gate *gate = &comp->entity->gates.at(selected_gate.at(1));

    sym_current = map_symbol(comp, gate);
    if (!sym_current) {
        return ToolResponse::revert();
    }
    sym_current->placement.shift = args.coords;

    selection.clear();
    selection.emplace(sym_current->uuid, ObjectType::SCHEMATIC_SYMBOL);

    move_init(args.coords);

    imp->tool_bar_set_actions({
            {InToolActionID::LMB},
            {InToolActionID::RMB},
            {InToolActionID::ROTATE},
            {InToolActionID::MIRROR},
            {InToolActionID::EDIT, "change symbol"},
    });

    return ToolResponse();
}

ToolResponse ToolMapSymbol::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        move_do_cursor(args.coords);
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB: {
            doc.c->get_schematic()->autoconnect_symbol(doc.c->get_sheet(), sym_current);
            if (sym_current->component->connections.size() == 0) {
                doc.c->get_schematic()->place_bipole_on_line(doc.c->get_sheet(), sym_current);
            }
            UUIDPath<2> selected_gate;
            if (data_mode == false) {
                gates_out.erase(UUIDPath<2>(sym_current->component->uuid, sym_current->gate->uuid));

                if (gates_out.size() == 0) {
                    return ToolResponse::commit();
                }
                else if (gates_out.size() == 1) {
                    selected_gate = gates_out.begin()->first;
                }
                else {
                    if (auto r = imp->dialogs.map_symbol(gates_out)) {
                        selected_gate = *r;
                    }
                    else {
                        return ToolResponse::commit();
                    }
                }
            }
            else {
                gates_from_data.pop_front();
                if (gates_from_data.size() == 0) {
                    return ToolResponse::commit();
                }
                selected_gate = gates_from_data.front();
            }
            Schematic *sch = doc.c->get_schematic();

            Component *comp = &sch->block->components.at(selected_gate.at(0));
            const Gate *gate = &comp->entity->gates.at(selected_gate.at(1));

            sym_current = map_symbol(comp, gate);
            if (!sym_current) {
                return ToolResponse::revert();
            }
            sym_current->placement.shift = args.coords;

            selection.clear();
            selection.emplace(sym_current->uuid, ObjectType::SCHEMATIC_SYMBOL);
        } break;

        case InToolActionID::RMB:
            doc.c->delete_schematic_symbol(sym_current->uuid);
            sym_current = nullptr;
            return ToolResponse::commit();
            break;

        case InToolActionID::CANCEL:
            return ToolResponse::revert();

        case InToolActionID::ROTATE:
        case InToolActionID::MIRROR:
            move_mirror_or_rotate(sym_current->placement.shift, args.action == InToolActionID::ROTATE);
            break;

        case InToolActionID::EDIT:
            change_symbol(sym_current);
            break;

        default:;
        }
    }
    return ToolResponse();
}
} // namespace horizon
