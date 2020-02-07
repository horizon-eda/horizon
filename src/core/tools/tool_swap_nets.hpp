#pragma once
#include "core/tool.hpp"

namespace horizon {

class ToolSwapNets : public ToolBase {
public:
    ToolSwapNets(IDocument *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    bool is_specific() override
    {
        return true;
    }

private:
    std::set<UUID> get_net_segments();
};
} // namespace horizon
