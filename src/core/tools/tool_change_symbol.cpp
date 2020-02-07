#include "tool_change_symbol.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>

namespace horizon {

ToolChangeSymbol::ToolChangeSymbol(IDocument *c, ToolID tid)
    : ToolBase(c, tid), ToolHelperGetSymbol(c, tid), ToolHelperMapSymbol(c, tid)

{
}


bool ToolChangeSymbol::can_begin()
{
    return get_symbol();
}

ToolResponse ToolChangeSymbol::begin(const ToolArgs &args)
{
    SchematicSymbol *sym = get_symbol();
    if (!sym) {
        return ToolResponse::end();
    }
    bool auto_selected = false;
    auto new_sym = get_symbol_for_unit(sym->gate->unit->uuid, &auto_selected);
    if (auto_selected == true) {
        imp->tool_bar_flash("No alternate symbols available");
        return ToolResponse::end();
    }
    if (!new_sym) {
        return ToolResponse::end();
    }
    for (const auto &it : sym->pool_symbol->pins) {
        if (new_sym->pins.count(it.first) == 0) {
            imp->tool_bar_flash("Pin " + sym->pool_symbol->unit->pins.at(it.first).primary_name
                                + " not found on new symbol");
            return ToolResponse::end();
        }
    }

    sym->pool_symbol = new_sym;
    return ToolResponse::commit();
}
ToolResponse ToolChangeSymbol::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
