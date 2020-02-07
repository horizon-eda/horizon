#pragma once
#include "core/tool.hpp"
#include "tool_helper_line_width_setting.hpp"

namespace horizon {

class ToolDrawLineRectangle : public ToolHelperLineWidthSetting {
public:
    ToolDrawLineRectangle(IDocument *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    void apply_settings() override;

private:
    std::set<class Line *> lines;
    enum class Mode { CENTER, CORNER };

    Mode mode = Mode::CENTER;
    Coordi first_pos;
    Coordi second_pos;
    int step = 0;

    std::array<class Junction *, 4> junctions;

    void update_junctions();
    void update_tip();
};
} // namespace horizon
