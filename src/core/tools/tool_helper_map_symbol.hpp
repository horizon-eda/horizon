#pragma once
#include "core/tool.hpp"

namespace horizon {
class ToolHelperMapSymbol : public virtual ToolBase {
public:
    using ToolBase::ToolBase;

protected:
    class SchematicSymbol *map_symbol(class Component *c, const class Gate *g);
    const class Symbol *get_symbol_for_unit(const UUID &unit_uu, bool *auto_selected = nullptr);
    void change_symbol(class SchematicSymbol *schsym);

private:
    std::map<UUID, UUID> selected_symbols;
};
} // namespace horizon
