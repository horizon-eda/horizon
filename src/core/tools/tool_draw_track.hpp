#pragma once
#include "core/tool.hpp"

namespace horizon {

class ToolDrawTrack : public ToolBase {
public:
    using ToolBase::ToolBase;
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {I::LMB, I::CANCEL, I::RMB, I::TOGGLE_ARC, I::FLIP_ARC};
    }

private:
    enum class ArcMode { OFF, NEXT, CURRENT };
    ArcMode arc_mode = ArcMode::OFF;
    class BoardJunction *temp_junc = 0;
    class Track *temp_track = 0;
    void create_temp_track();
    class BoardRules *rules;

    void update_tip();
    ToolResponse finish();
};
} // namespace horizon
