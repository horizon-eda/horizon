#pragma once
#include "core/tool.hpp"
#include "tool_helper_merge.hpp"
#include "tool_helper_move.hpp"

namespace horizon {

class ToolMove : public ToolHelperMove, public ToolHelperMerge {
public:
    ToolMove(IDocument *c, ToolID tid);
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
                I::LMB_RELEASE,
                I::CANCEL,
                I::COMMIT,
                I::RMB,
                I::MIRROR,
                I::MIRROR_CURSOR,
                I::ROTATE,
                I::ROTATE_CURSOR,
                I::RESTRICT,
                I::MOVE_UP,
                I::MOVE_DOWN,
                I::MOVE_LEFT,
                I::MOVE_RIGHT,
                I::MOVE_UP_FINE,
                I::MOVE_DOWN_FINE,
                I::MOVE_LEFT_FINE,
                I::MOVE_RIGHT_FINE,
        };
    }

private:
    Coordi get_selection_center();
    void expand_selection();
    void update_tip();
    void do_move(const Coordi &c);

    void collect_nets();
    std::set<UUID> nets;

    bool update_airwires = true;
    void finish();
    bool is_key = false;
    Coordi key_delta;

    std::set<class Plane *> planes;
};
} // namespace horizon
