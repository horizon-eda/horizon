#pragma once

#include "core/tool_pub.hpp"

namespace horizon {

class ToolEditTable : public ToolBase {
public:
    using ToolBase::ToolBase;
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;

    bool is_specific() override
    {
        return true;
    }
};

} // namespace horizon
