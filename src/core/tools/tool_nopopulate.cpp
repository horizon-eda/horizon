#include "tool_nopopulate.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"
#include <iostream>
#include "core/tool_id.hpp"

namespace horizon {

ToolNoPopulate::ToolNoPopulate(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolNoPopulate::can_begin()
{
    for (const auto &it : selection) {
        if (it.type == ObjectType::SCHEMATIC_SYMBOL) {
            auto sym = doc.c->get_schematic_symbol(it.uuid);
            if (sym->component->nopopulate == (tool_id == ToolID::POPULATE))
                return true;
        }
    }
    return false;
}

ToolResponse ToolNoPopulate::begin(const ToolArgs &args)
{
    for (const auto &it : args.selection) {
        if (it.type == ObjectType::SCHEMATIC_SYMBOL) {
            auto sym = doc.c->get_schematic_symbol(it.uuid);
            sym->component->nopopulate = (tool_id == ToolID::NOPOPULATE);
        }
    }
    return ToolResponse::commit();
}
ToolResponse ToolNoPopulate::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
