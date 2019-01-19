#pragma once
#include "core/core.hpp"

namespace horizon {
class ToolHelperGetSymbol : public virtual ToolBase {
public:
    ToolHelperGetSymbol(Core *c, ToolID tid) : ToolBase(c, tid)
    {
    }

protected:
    class SchematicSymbol *get_symbol();
};
} // namespace horizon
