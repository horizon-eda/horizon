#pragma once
#include "core/tool.hpp"

namespace horizon {

class ToolEditPadParameterSet : public ToolBase {
public:
    using ToolBase::ToolBase;
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    bool is_specific() override
    {
        return true;
    }
    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::LMB,
        };
    }

private:
    std::set<class Pad *> pads;
    void select_pads();
    class PadParameterSetWindow *win = nullptr;
    std::set<class Pad *> get_pads();
};
} // namespace horizon
