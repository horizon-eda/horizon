#pragma once
#include "core/tool.hpp"

namespace horizon {

class ToolSwapGates : public ToolBase {
public:
    ToolSwapGates(IDocument *c, ToolID tid);
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
