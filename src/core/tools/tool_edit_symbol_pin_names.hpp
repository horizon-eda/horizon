#pragma once
#include "core/tool.hpp"
#include "tool_helper_get_symbol.hpp"

namespace horizon {

class ToolEditSymbolPinNames : public ToolHelperGetSymbol {
public:
    using ToolHelperGetSymbol::ToolHelperGetSymbol;
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    bool is_specific() override
    {
        return true;
    }
    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::LMB,
        };
    }

private:
    class SymbolPinNamesWindow *win = nullptr;
    class SchematicSymbol *sym = nullptr;
};
} // namespace horizon
