#include "tool_nopopulate.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"
#include <iostream>
#include "core/tool_id.hpp"

namespace horizon {

bool ToolNoPopulate::can_begin()
{
    if (!doc.c)
        return false;
    if (!doc.c->in_hierarchy())
        return false;
    for (const auto &it : selection) {
        if (it.type == ObjectType::SCHEMATIC_SYMBOL) {
            const auto &comp = *doc.c->get_sheet()->symbols.at(it.uuid).component;
            const auto comp_info = doc.c->get_top_block()->get_component_info(comp, doc.c->get_instance_path());
            if (comp_info.nopopulate == (tool_id == ToolID::POPULATE))
                return true;
        }
    }
    return false;
}

ToolResponse ToolNoPopulate::begin(const ToolArgs &args)
{
    for (const auto &it : args.selection) {
        if (it.type == ObjectType::SCHEMATIC_SYMBOL) {
            auto &comp = *doc.c->get_sheet()->symbols.at(it.uuid).component;
            doc.c->get_top_block()->set_nopopulate(comp, doc.c->get_instance_path(), (tool_id == ToolID::NOPOPULATE));
        }
    }
    return ToolResponse::commit();
}
ToolResponse ToolNoPopulate::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
