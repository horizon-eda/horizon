#pragma once
#include "core/tool.hpp"
#include "tool_helper_line_width_setting.hpp"
#include "tool_settings_rectangle_mode.hpp"

namespace horizon {

class ToolDrawLineRectangle : public ToolHelperLineWidthSetting {
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
                I::LMB, I::CANCEL, I::RMB, I::RECTANGLE_MODE, I::ENTER_WIDTH,
        };
    }

    std::map<ToolID, ToolSettings *> get_all_settings() override;

private:
    std::set<class Line *> lines;
    ToolSettingsRectangleMode settings_rect;
    using Mode = ToolSettingsRectangleMode::Mode;

    Coordi first_pos;
    Coordi second_pos;
    int step = 0;

    std::array<class Junction *, 4> junctions;

    void update_junctions();
    void update_tip();
};
} // namespace horizon
