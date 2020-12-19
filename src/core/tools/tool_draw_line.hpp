#pragma once
#include "core/tool.hpp"
#include "tool_helper_line_width_setting.hpp"
#include "tool_helper_restrict.hpp"

namespace horizon {

class ToolDrawLine : public ToolHelperLineWidthSetting, public ToolHelperRestrict {
public:
    using ToolHelperLineWidthSetting::ToolHelperLineWidthSetting;
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::LMB, I::CANCEL, I::RMB, I::RESTRICT, I::ENTER_WIDTH,
        };
    }

    void apply_settings() override;

private:
    class Junction *temp_junc = 0;
    class Line *temp_line = 0;
    void update_tip();
    bool first_line = true;
    std::set<const Junction *> junctions_created;
    void do_move(const Coordi &c);
};
} // namespace horizon
