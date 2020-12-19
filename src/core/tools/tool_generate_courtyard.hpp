#pragma once
#include "core/tool.hpp"

namespace horizon {

class ToolGenerateCourtyard : public ToolBase {
public:
    using ToolBase::ToolBase;
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    bool is_specific() override
    {
        return false;
    }
};
} // namespace horizon
