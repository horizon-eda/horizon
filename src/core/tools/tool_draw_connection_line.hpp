#pragma once
#include "core/tool.hpp"
#include "tool_helper_pick_pad.hpp"

namespace horizon {

class ToolDrawConnectionLine : public virtual ToolBase, public ToolHelperPickPad {
public:
    using ToolBase::ToolBase;
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::LMB,
                I::CANCEL,
                I::RMB,
        };
    }

private:
    class BoardJunction *temp_junc = 0;
    class ConnectionLine *temp_line = 0;
    class ConnectionLine *temp_line_last = 0;
    class ConnectionLine *temp_line_last2 = 0;
    class Net *temp_net = nullptr;

    void update_tip();
};
} // namespace horizon
