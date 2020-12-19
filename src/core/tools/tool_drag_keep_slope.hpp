#pragma once
#include "core/tool.hpp"
#include "util/keep_slope_util.hpp"
#include <deque>

namespace horizon {

class ToolDragKeepSlope : public ToolBase {
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
    class TrackInfo : public KeepSlopeInfo {
    public:
        class Track &track; // the track itself

        TrackInfo(Track &tr, const Track &fr, const Track &to);
        // TrackInfo(Track *a, Track *b, Track *c): track(a), track_from(b),
        // track_to(c) {}
    };

    std::deque<TrackInfo> track_info;
    Coordi pos_orig;
};
} // namespace horizon
