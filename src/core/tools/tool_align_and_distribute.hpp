#pragma once
#include "core/tool.hpp"
#include "tool_helper_save_placements.hpp"

namespace horizon {

class ToolAlignAndDistribute : public virtual ToolBase, public ToolHelperSavePlacements {
public:
    using ToolBase::ToolBase;
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    bool is_specific() override
    {
        return true;
    }
    std::set<InToolActionID> get_actions() const override
    {
        return {};
    }

    enum class Operation {
        RESET,
        DISTRIBUTE_ORIGIN_HORIZONTAL,
        DISTRIBUTE_ORIGIN_VERTICAL,
        ALIGN_ORIGIN_TOP,
        ALIGN_ORIGIN_LEFT,
        ALIGN_ORIGIN_RIGHT,
        ALIGN_ORIGIN_BOTTOM,
        ALIGN_BBOX_TOP,
        ALIGN_BBOX_LEFT,
        ALIGN_BBOX_RIGHT,
        ALIGN_BBOX_BOTTOM,
        DISTRIBUTE_EQUIDISTANT_HORIZONTAL,
        DISTRIBUTE_EQUIDISTANT_VERTICAL,
    };


private:
    class AlignAndDistributeWindow *win = nullptr;
    void align(Coordi::type Coordi::*cp, const int64_t PlacementInfo::*pp, int64_t pp_sign);
    void distribute(Coordi::type Coordi::*cp);
    void distribute_equi(Coordi::type Coordi::*cp, const int64_t PlacementInfo::*pp1,
                         const int64_t PlacementInfo::*pp2);
};
} // namespace horizon
