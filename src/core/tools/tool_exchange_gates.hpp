#pragma once
#include "core/tool.hpp"

namespace horizon {

class ToolExchangeGates : public ToolBase {
public:
    using ToolBase::ToolBase;
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    bool is_specific() override
    {
        return true;
    }

private:
    std::pair<class SchematicSymbol *, SchematicSymbol *> get_symbols();
};
} // namespace horizon
