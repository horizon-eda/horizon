#pragma once
#include "core/core.hpp"

namespace horizon {
class ToolHelperGetSymbol : public virtual ToolBase {

protected:
    class SchematicSymbol *get_symbol();
};
} // namespace horizon
