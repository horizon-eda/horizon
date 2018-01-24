#include "tool_add_part.hpp"
#include "core_schematic.hpp"
#include "imp/imp_interface.hpp"
#include "pool/part.hpp"
#include <iostream>

namespace horizon {

ToolAddPart::ToolAddPart(Core *c, ToolID tid) : ToolBase(c, tid), ToolHelperMapSymbol(c, tid), ToolHelperMove(c, tid)
{
}

bool ToolAddPart::can_begin()
{
    return core.c;
}

ToolResponse ToolAddPart::begin(const ToolArgs &args)
{
    std::cout << "tool add part\n";
    Schematic *sch = core.c->get_schematic();

    bool r;
    if (tool_id == ToolID::ADD_PART) {
        UUID part_uuid = imp->take_part();
        if (!part_uuid) {
            std::tie(r, part_uuid) = imp->dialogs.select_part(core.r->m_pool, UUID(), UUID());
            if (!r) {
                return ToolResponse::end();
            }
        }
        imp->part_placed(part_uuid);
        auto part = core.c->m_pool->get_part(part_uuid);

        auto uu = UUID::random();
        comp = &sch->block->components.emplace(uu, uu).first->second;
        comp->entity = part->entity;
        comp->part = part;
    }
    else {
        UUID entity_uuid;
        std::tie(r, entity_uuid) = imp->dialogs.select_entity(core.r->m_pool);
        if (!r) {
            return ToolResponse::end();
        }
        auto uu = UUID::random();
        comp = &sch->block->components.emplace(uu, uu).first->second;
        comp->entity = core.c->m_pool->get_entity(entity_uuid);
    }
    comp->refdes = comp->entity->prefix + "?";

    for (auto &it : comp->entity->gates) {
        gates.push_back(&it.second);
    }
    std::sort(gates.begin(), gates.end(), [](auto &a, auto &b) { return a->suffix < b->suffix; });

    sym_current = map_symbol(comp, gates.front());
    if (!sym_current) {
        core.r->revert();
        return ToolResponse::end();
    }
    sym_current->placement.shift = args.coords;
    core.c->selection.clear();
    core.c->selection.emplace(sym_current->uuid, ObjectType::SCHEMATIC_SYMBOL);
    move_init(args.coords);
    current_gate = 0;

    return ToolResponse();
}

void ToolAddPart::update_tip()
{
    std::stringstream ss;
    ss << "<b>LMB:</b>place <b>RMB:</b>cancel <b>r:</b>rotate <b>e:</b>mirror "
          "<i>placing gate ";
    ss << current_gate + 1 << "/" << gates.size() << "</i>";
    imp->tool_bar_set_tip(ss.str());
}

ToolResponse ToolAddPart::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        move_do_cursor(args.coords);
    }
    else if (args.type == ToolEventType::CLICK) {
        if (args.button == 1) {
            if (current_gate + 1 == gates.size()) { // last gate
                core.r->commit();
                return ToolResponse::end();
            }
            else { // place next gate
                auto sym = map_symbol(comp, gates.at(current_gate + 1));
                if (sym) {
                    sym_current = sym;
                    current_gate++;
                    sym_current->placement.shift = args.coords;
                    core.c->selection.clear();
                    core.c->selection.emplace(sym_current->uuid, ObjectType::SCHEMATIC_SYMBOL);
                    return ToolResponse();
                }
            }
        }
        else {
            if (current_gate == 0) {
                core.r->revert();
                return ToolResponse::end();
            }
            else {
                core.c->delete_schematic_symbol(sym_current->uuid);
                sym_current = nullptr;
                core.r->commit();
                return ToolResponse::end();
            }
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
