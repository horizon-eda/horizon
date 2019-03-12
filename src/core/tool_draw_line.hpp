#pragma once
#include "core.hpp"
#include "tool_helper_line_width_setting.hpp"
#include "tool_helper_restrict.hpp"

namespace horizon {

class ToolDrawLine : public ToolHelperLineWidthSetting, public ToolHelperRestrict {
public:
    ToolDrawLine(Core *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    bool handles_esc() override
    {
        return true;
    }

    void apply_settings() override;

private:
    Junction *temp_junc = 0;
    Line *temp_line = 0;
    void update_tip();
    bool first_line = true;
    std::set<const Junction *> junctions_created;
    void do_move(const Coordi &c);
};
} // namespace horizon
