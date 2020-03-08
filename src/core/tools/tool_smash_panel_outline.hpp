#pragma once
#include "core/tool.hpp"

namespace horizon {

class ToolSmashPanelOutline : public ToolBase {
public:
    ToolSmashPanelOutline(IDocument *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    bool is_specific() override
    {
        return true;
    }

private:
};
} // namespace horizon
