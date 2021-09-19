#pragma once
#include "core/tool.hpp"
#include "canvas/selectables.hpp"
#include "util/placement.hpp"
#include "tool_helper_collect_nets.hpp"
#include "tool_helper_save_placements.hpp"

namespace horizon {

class ToolRotateArbitrary : public virtual ToolBase, public ToolHelperCollectNets, public ToolHelperSavePlacements {
public:
    using ToolBase::ToolBase;
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    bool is_specific() override
    {
        return true;
    }
    ~ToolRotateArbitrary();
    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::LMB, I::CANCEL, I::RMB, I::ENTER_DATUM, I::TOGGLE_ANGLE_SNAP,
        };
    }

private:
    Coordi origin;
    Coordi ref;
    int iangle = 0;
    bool snap = true;
    double scale = 1;
    void expand_selection();
    void update_tip();
    void apply_placements_rotation(int angle);
    void apply_placements_scale(double sc);
    enum class State { ORIGIN, ROTATE, REF, SCALE };
    State state = State::ORIGIN;
    class CanvasAnnotation *annotation = nullptr;
    std::set<UUID> nets;
    void update_airwires(bool fast);
};
} // namespace horizon
