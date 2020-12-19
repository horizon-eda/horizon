#pragma once
#include "core/tool.hpp"

namespace horizon {

class ToolManageBuses : public ToolBase {
public:
    using ToolBase::ToolBase;
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;

private:
};
} // namespace horizon
