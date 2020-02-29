#pragma once
#include "core/tool.hpp"
#include "tool_helper_merge.hpp"
#include "tool_helper_move.hpp"

namespace horizon {

class ToolMove : public ToolHelperMove, public ToolHelperMerge {
public:
    ToolMove(IDocument *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    bool is_specific() override
    {
        return true;
    }
    bool handles_esc() override
    {
        return true;
    }

private:
    Coordi get_selection_center();
    void expand_selection();
    void update_tip();
    void do_move(const Coordi &c);

    void collect_nets();
    std::set<UUID> nets;

    bool update_airwires = true;
    void finish();
    bool is_key = false;
    Coordi key_delta;

    std::set<class Plane *> planes;
};
} // namespace horizon
