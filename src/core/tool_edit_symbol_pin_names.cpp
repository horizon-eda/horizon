#include "tool_edit_symbol_pin_names.hpp"
#include "core_schematic.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>

namespace horizon {

ToolEditSymbolPinNames::ToolEditSymbolPinNames(Core *c, ToolID tid) : ToolBase(c, tid), ToolHelperGetSymbol(c, tid)
{
}

bool ToolEditSymbolPinNames::can_begin()
{
    return get_symbol();
}

ToolResponse ToolEditSymbolPinNames::begin(const ToolArgs &args)
{
    SchematicSymbol *sym = get_symbol();
    if (!sym) {
        return ToolResponse::end();
    }
    bool r = imp->dialogs.edit_symbol_pin_names(sym);
    if (r) {
        core.r->commit();
    }
    else {
        core.r->revert();
    }
    return ToolResponse::end();
}
ToolResponse ToolEditSymbolPinNames::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
