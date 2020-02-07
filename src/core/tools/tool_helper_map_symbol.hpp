#pragma once
#include "core/tool.hpp"

namespace horizon {
class ToolHelperMapSymbol : public virtual ToolBase {
public:
    ToolHelperMapSymbol(IDocument *c, ToolID tid) : ToolBase(c, tid)
    {
    }

protected:
    class SchematicSymbol *map_symbol(class Component *c, const class Gate *g);
    const class Symbol *get_symbol_for_unit(const UUID &unit_uu, bool *auto_selected = nullptr);
};
} // namespace horizon
