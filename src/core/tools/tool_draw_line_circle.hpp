#pragma once
#include "tool_helper_line_width_setting.hpp"

namespace horizon {

class ToolDrawLineCircle : public ToolHelperLineWidthSetting {
public:
    ToolDrawLineCircle(IDocument *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::LMB, I::CANCEL, I::RMB, I::ENTER_DATUM, I::ENTER_WIDTH,
        };
    }
    void apply_settings() override;


private:
    Coordi first_pos;
    Coordi second_pos;

    int step = 0;
    std::array<class Junction *, 3> junctions = {};
    std::array<class Arc *, 2> arcs = {};
    void create();

    void update_junctions();
    void update_tip();
};
} // namespace horizon
