#pragma once
#include "core/tool.hpp"
#include "canvas/selectables.hpp"
#include "util/placement.hpp"

namespace horizon {

class ToolRotateArbitrary : public ToolBase {
public:
    ToolRotateArbitrary(IDocument *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    bool is_specific() override
    {
        return true;
    }
    ~ToolRotateArbitrary();

private:
    Coordi origin;
    Coordi ref;
    int iangle = 0;
    bool snap = true;
    double scale = 1;
    void expand_selection();
    void update_tip();
    void save_placements();
    void apply_placements_rotation(int angle);
    void apply_placements_scale(double sc);
    enum class State { ORIGIN, ROTATE, REF, SCALE };
    State state = State::ORIGIN;
    std::map<SelectableRef, Placement> placements;
    class CanvasAnnotation *annotation = nullptr;
};
} // namespace horizon
