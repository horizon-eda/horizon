#pragma once
#include "core/tool.hpp"
#include "core/core_board.hpp"
#include "util/keep_slope_util.hpp"
#include <deque>

namespace horizon {

class ToolDragRound : public ToolBase {
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
    struct RoundInfo {
    public:
        class Track *first, *second, *arc;
        class BoardJunction *junc, *newjunc;

        Coordi orig;
        double angle_first, angle_second, axis;
        double inv_cosine;
        double offset;

        void apply(double distance) const;

        static RoundInfo setup(Board *board, Track *first, Track *second, BoardJunction *junc);
    };
    std::set<UUID> nets;

    std::deque<RoundInfo> round_info;
    Coordi pos_orig;
};
} // namespace horizon
