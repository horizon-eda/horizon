#pragma once
#include "tool_helper_get_symbol.hpp"

namespace horizon {
class ToolEditCustomValue : public ToolHelperGetSymbol {
public:
    using ToolHelperGetSymbol::ToolHelperGetSymbol;
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    bool is_specific() override
    {
        return true;
    }
};
} // namespace horizon
