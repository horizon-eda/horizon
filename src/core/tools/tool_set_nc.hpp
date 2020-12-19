#pragma once
#include "core/tool.hpp"

namespace horizon {

class ToolSetNotConnected : public ToolBase {
public:
    using ToolBase::ToolBase;
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    bool is_specific() override
    {
        return false;
    }
    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::LMB,
                I::CANCEL,
                I::RMB,
                I::NC_MODE,
        };
    }

private:
    enum class Mode { SET, CLEAR, TOGGLE };
    Mode mode = Mode::SET;
    void update_tip();
};
} // namespace horizon
