#pragma once
#include "core/tool.hpp"
#include "tool_helper_line_width_setting.hpp"

namespace horizon {

class ToolDrawArc : public ToolHelperLineWidthSetting {
public:
    ToolDrawArc(IDocument *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    void apply_settings() override;

private:
    enum class DrawArcState { FROM, TO, CENTER };
    DrawArcState state = DrawArcState::FROM;
    class Junction *temp_junc = 0;
    Junction *from_junc = 0;
    Junction *to_junc = 0;
    class Arc *temp_arc = 0;
    Junction *make_junction(const Coordi &coords);
    void update_tip();
};
} // namespace horizon
