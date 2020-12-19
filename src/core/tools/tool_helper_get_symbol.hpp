#pragma once
#include "core/tool.hpp"

namespace horizon {
class ToolHelperGetSymbol : public virtual ToolBase {
public:
    using ToolBase::ToolBase;


protected:
    class SchematicSymbol *get_symbol();
};
} // namespace horizon
