#pragma once
#include "core/tool.hpp"
#include "board/track.hpp"
#include "tool_helper_pick_pad.hpp"

namespace horizon {

class ToolMoveTrackConnection : public virtual ToolBase, public ToolHelperPickPad {
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
                I::CANCEL,
                I::RMB,
        };
    }

private:
    Track *track = nullptr;
    Track::Connection *conn = nullptr;

    void move(const Coordi &c);
};
} // namespace horizon
