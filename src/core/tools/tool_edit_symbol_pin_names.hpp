#pragma once
#include "block/component.hpp"
#include "core/tool.hpp"
#include <forward_list>
#include "tool_helper_get_symbol.hpp"

namespace horizon {

class ToolEditSymbolPinNames : public ToolHelperGetSymbol {
public:
    ToolEditSymbolPinNames(IDocument *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    bool is_specific() override
    {
        return true;
    }

private:
    class SymbolPinNamesWindow *win = nullptr;
    class SchematicSymbol *sym = nullptr;
};
} // namespace horizon
