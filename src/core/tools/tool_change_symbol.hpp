#pragma once
#include "core/tool.hpp"
#include "tool_helper_get_symbol.hpp"
#include "tool_helper_map_symbol.hpp"

namespace horizon {

class ToolChangeSymbol : public ToolHelperGetSymbol, public ToolHelperMapSymbol {
public:
    ToolChangeSymbol(IDocument *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    bool is_specific() override
    {
        return true;
    }
};
} // namespace horizon
