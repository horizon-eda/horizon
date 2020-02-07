#include "tool_assign_part.hpp"
#include "tool_add_part.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"
#include "imp/imp_interface.hpp"
#include "pool/part.hpp"
#include <iostream>

namespace horizon {

ToolAssignPart::ToolAssignPart(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolAssignPart::can_begin()
{
    return get_entity() != nullptr;
}

const Entity *ToolAssignPart::get_entity()
{
    const Entity *entity = nullptr;
    for (const auto &it : selection) {
        if (it.type == ObjectType::SCHEMATIC_SYMBOL) {
            auto sym = doc.c->get_schematic_symbol(it.uuid);
            if (entity) {
                if (entity != sym->component->entity) {
                    return nullptr;
                }
            }
            else {
                entity = sym->component->entity;
                comp = sym->component;
            }
        }
    }
    return entity;
}

ToolResponse ToolAssignPart::begin(const ToolArgs &args)
{
    std::cout << "tool assing part\n";
    const Entity *entity = get_entity();

    if (!entity) {
        return ToolResponse::end();
    }
    UUID part_uuid;
    if (auto data = dynamic_cast<const ToolAddPart::ToolDataAddPart *>(args.data.get())) {
        part_uuid = data->part_uuid;
    }
    if (!part_uuid) {
        auto current_part_uuid = comp->part ? comp->part->uuid : UUID();
        auto r = imp->dialogs.select_part(doc.r->get_pool(), entity->uuid, current_part_uuid, true);
        if (r.first) {
            part_uuid = r.second;
        }
    }
    if (part_uuid) {
        auto part = doc.r->get_pool()->get_part(part_uuid);
        if (part->entity->uuid != entity->uuid) {
            imp->tool_bar_flash("wrong entity");
            return ToolResponse::end();
        }

        for (const auto &it : args.selection) {
            if (it.type == ObjectType::SCHEMATIC_SYMBOL) {
                auto sym = doc.c->get_schematic_symbol(it.uuid);
                if (sym->component->entity == entity) {
                    sym->component->part = part;
                }
            }
        }

        return ToolResponse::commit();
    }
    return ToolResponse::end();
}
ToolResponse ToolAssignPart::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
