#pragma once
#include "core/tool.hpp"
#include "tool_settings_rectangle_mode.hpp"

namespace horizon {

class ToolDrawPolygonRectangle : public ToolBase {
public:
    using ToolBase::ToolBase;
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;

    ToolSettings *get_settings() override
    {
        return &settings;
    }

    void apply_settings() override;

    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::LMB,
                I::CANCEL,
                I::RMB,
                I::RECTANGLE_MODE,
                I::POLYGON_CORNER_RADIUS,
                I::POLYGON_DECORATION_POSITION,
                I::POLYGON_DECORATION_SIZE,
                I::POLYGON_DECORATION_STYLE,
        };
    }

private:
    ToolSettingsRectangleMode settings;
    using Mode = ToolSettingsRectangleMode::Mode;

    enum class Decoration { NONE, CHAMFER, NOTCH };

    Decoration decoration = Decoration::NONE;
    int decoration_pos = 0;
    Coordi first_pos;
    Coordi second_pos;
    int step = 0;
    uint64_t decoration_size = 1.2_mm;
    int64_t corner_radius = 0;

    class Polygon *temp = nullptr;

    void update_polygon();
    void update_tip();
};
} // namespace horizon
