#include "tool_add_part.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"
#include "imp/imp_interface.hpp"
#include "pool/part.hpp"
#include <iostream>
#include <sstream>
#include "core/tool_id.hpp"
#include "pool/pool.hpp"

namespace horizon {

ToolAddPart::ToolAddPart(IDocument *c, ToolID tid)
    : ToolBase(c, tid), ToolHelperMapSymbol(c, tid), ToolHelperMove(c, tid)

{
}

bool ToolAddPart::can_begin()
{
    return doc.c;
}

UUID ToolAddPart::create_tag()
{
    auto uu = UUID::random();
    auto block = doc.c->get_block();
    block->tag_names[uu] = std::to_string(block->tag_names.size());
    return uu;
}

ToolResponse ToolAddPart::begin(const ToolArgs &args)
{
    std::cout << "tool add part\n";
    Schematic *sch = doc.c->get_schematic();

    if (tool_id == ToolID::ADD_PART) {
        UUID part_uuid;
        if (auto data = dynamic_cast<const ToolDataAddPart *>(args.data.get())) {
            part_uuid = data->part_uuid;
        }
        if (!part_uuid) {
            if (auto r = imp->dialogs.select_part(doc.r->get_pool(), UUID(), UUID())) {
                part_uuid = *r;
            }
            else {
                return ToolResponse::end();
            }
        }
        imp->part_placed(part_uuid);
        auto part = doc.c->get_pool().get_part(part_uuid);

        auto uu = UUID::random();
        comp = &sch->block->components.emplace(uu, uu).first->second;
        comp->entity = part->entity;
        comp->part = part;
    }
    else {
        if (auto r = imp->dialogs.select_entity(doc.r->get_pool())) {
            auto uu = UUID::random();
            comp = &sch->block->components.emplace(uu, uu).first->second;
            comp->entity = doc.c->get_pool().get_entity(*r);
        }
        else {
            return ToolResponse::end();
        }
    }
    comp->refdes = comp->entity->prefix + "?";
    comp->tag = create_tag();

    for (auto &it : comp->entity->gates) {
        gates.push_back(&it.second);
    }
    std::sort(gates.begin(), gates.end(), [](auto &a, auto &b) { return a->suffix < b->suffix; });

    sym_current = map_symbol(comp, gates.front());
    if (!sym_current) {
        return ToolResponse::revert();
    }
    sym_current->placement.shift = args.coords;
    selection.clear();
    selection.emplace(sym_current->uuid, ObjectType::SCHEMATIC_SYMBOL);
    move_init(args.coords);
    current_gate = 0;
    update_tip();

    return ToolResponse();
}

void ToolAddPart::update_tip()
{
    imp->tool_bar_set_actions({
            {InToolActionID::LMB, "place"},
            {InToolActionID::RMB, "cancel"},
            {InToolActionID::ROTATE},
            {InToolActionID::MIRROR},
    });

    std::stringstream ss;
    ss << "placing gate ";
    ss << current_gate + 1 << "/" << gates.size();
    imp->tool_bar_set_tip(ss.str());
}

ToolResponse ToolAddPart::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        move_do_cursor(args.coords);
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB:
            doc.c->get_schematic()->autoconnect_symbol(doc.c->get_sheet(), sym_current);
            if (sym_current->component->connections.size() == 0) {
                doc.c->get_schematic()->place_bipole_on_line(doc.c->get_sheet(), sym_current);
            }
            if (current_gate + 1 == gates.size()) { // last gate
                current_gate = 0;
                // add next component
                auto last_comp = comp;
                auto uu = UUID::random();
                Schematic *sch = doc.c->get_schematic();
                comp = &sch->block->components.emplace(uu, uu).first->second;
                comp->entity = last_comp->entity;
                comp->part = last_comp->part;
                comp->refdes = comp->entity->prefix + "?";
                comp->tag = create_tag();

                auto old_symbol = sym_current;
                sym_current = map_symbol(comp, gates.front());
                if (!sym_current) {
                    doc.c->get_block()->components.erase(comp->uuid);
                    return ToolResponse::commit();
                }
                sym_current->placement = old_symbol->placement;
                sym_current->placement.shift = args.coords;
                sym_current->symbol.apply_placement(sym_current->placement);

                selection.clear();
                selection.emplace(sym_current->uuid, ObjectType::SCHEMATIC_SYMBOL);

                return ToolResponse();
            }
            else { // place next gate
                auto sym = map_symbol(comp, gates.at(current_gate + 1));
                if (sym) {
                    sym->placement = sym_current->placement;
                    sym_current = sym;
                    current_gate++;
                    sym_current->placement.shift = args.coords;
                    sym_current->symbol.apply_placement(sym_current->placement);
                    selection.clear();
                    selection.emplace(sym_current->uuid, ObjectType::SCHEMATIC_SYMBOL);
                    return ToolResponse();
                }
            }
            break;

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            if (current_gate == 0) { // also delete the component
                doc.c->get_block()->components.erase(comp->uuid);
            }
            doc.c->delete_schematic_symbol(sym_current->uuid);
            sym_current = nullptr;
            return ToolResponse::commit();
            break;

        case InToolActionID::MIRROR:
        case InToolActionID::ROTATE:
            move_mirror_or_rotate(sym_current->placement.shift, args.action == InToolActionID::ROTATE);
            break;

        default:;
        }
    }
    update_tip();
    return ToolResponse();
}
} // namespace horizon
