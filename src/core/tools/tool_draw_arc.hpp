#pragma once
#include "core/tool.hpp"
#include "tool_helper_line_width_setting.hpp"

namespace horizon {

class ToolDrawArc : public ToolHelperLineWidthSetting {
public:
    using ToolHelperLineWidthSetting::ToolHelperLineWidthSetting;
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    void apply_settings() override;
    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::LMB, I::CANCEL, I::RMB, I::ENTER_WIDTH, I::FLIP_ARC, I::ARC_MODE,
        };
    }
    ~ToolDrawArc();

private:
    enum class State { FROM, TO, CENTER, CENTER_START, RADIUS, START_ANGLE, END_ANGLE };
    State state;
    class Junction *temp_junc = 0;
    Junction *from_junc = 0;
    Junction *to_junc = 0;
    class Arc *temp_arc = 0;
    Junction *make_junction(const Coordd &coords);
    void update_tip();
    double radius = 0;
    double start_angle = 0;
    void set_radius_angle(double r, double a, double b);
    void update_end_angle(const Coordi &c);
    bool flipped = false;
    class CanvasAnnotation *annotation = nullptr;
};
} // namespace horizon
