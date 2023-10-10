#pragma once
#include "core/tool.hpp"

namespace horizon {

class ToolSwapPlacement : public ToolBase {
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
    std::pair<class BoardPackage *, BoardPackage *> get_packages();
};
} // namespace horizon
