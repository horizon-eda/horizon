#pragma once
#include "core/tool.hpp"
#include "tool_helper_collect_nets.hpp"

namespace horizon {

class ToolEnterDatum : public virtual ToolBase, public ToolHelperCollectNets {
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
};
} // namespace horizon
