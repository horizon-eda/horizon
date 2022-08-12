#include "tool_clear_part.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"
#include "imp/imp_interface.hpp"

namespace horizon {

bool ToolClearPart::can_begin()
{
    for (const auto &it : selection) {
        if (it.type == ObjectType::SCHEMATIC_SYMBOL) {
            auto &sym = doc.c->get_sheet()->symbols.at(it.uuid);
            if (sym.component->part)
                return true;
        }
    }
    return false;
}

ToolResponse ToolClearPart::begin(const ToolArgs &args)
{
    for (const auto &it : args.selection) {
        if (it.type == ObjectType::SCHEMATIC_SYMBOL) {
            auto &sym = doc.c->get_sheet()->symbols.at(it.uuid);
            sym.component->part = nullptr;
        }
    }
    return ToolResponse::commit();
}

ToolResponse ToolClearPart::update(const ToolArgs &args)
{
    return ToolResponse();
}

} // namespace horizon
