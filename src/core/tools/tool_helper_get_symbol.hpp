#pragma once
#include "core/tool.hpp"

namespace horizon {
class ToolHelperGetSymbol : public virtual ToolBase {
public:
    ToolHelperGetSymbol(IDocument *c, ToolID tid) : ToolBase(c, tid)
    {
    }

protected:
    class SchematicSymbol *get_symbol();
};
} // namespace horizon
