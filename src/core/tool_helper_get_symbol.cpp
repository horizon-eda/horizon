#include "tool_helper_get_symbol.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"

namespace horizon {
SchematicSymbol *ToolHelperGetSymbol::get_symbol()
{
    SchematicSymbol *rsym = nullptr;
    for (const auto &it : selection) {
        if (it.type == ObjectType::SCHEMATIC_SYMBOL) {
            auto sym = doc.c->get_schematic_symbol(it.uuid);
            if (rsym) {
                if (rsym != sym) {
                    return nullptr;
                }
            }
            else {
                rsym = sym;
            }
        }
    }
    return rsym;
}
} // namespace horizon
