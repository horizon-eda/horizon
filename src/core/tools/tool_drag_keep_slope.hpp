#pragma once
#include "board/track.hpp"
#include "core/tool.hpp"
#include <deque>

namespace horizon {

class ToolDragKeepSlope : public ToolBase {
public:
    ToolDragKeepSlope(IDocument *c, ToolID tid);
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
    class TrackInfo {
    public:
        Track *track = nullptr; // the track itself
        Track *track_from = nullptr;
        Track *track_to = nullptr;
        Coordi pos_from2;
        Coordi pos_to2;
        Coordi pos_from_orig;
        Coordi pos_to_orig;
        TrackInfo(Track *tr, Track *fr, Track *to);
        // TrackInfo(Track *a, Track *b, Track *c): track(a), track_from(b),
        // track_to(c) {}
    };

    std::deque<TrackInfo> track_info;
    Coordi pos_orig;
};
} // namespace horizon
