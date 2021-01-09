#pragma once
#include "core/tool.hpp"

namespace horizon {
class ToolPlaceDecal : public ToolBase {
public:
    using ToolBase::ToolBase;
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::LMB, I::CANCEL, I::RMB, I::ROTATE, I::MIRROR, I::ENTER_SIZE,
        };
    }

private:
    class BoardDecal *temp = nullptr;
};
} // namespace horizon
