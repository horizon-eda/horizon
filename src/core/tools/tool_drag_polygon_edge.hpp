#pragma once
#include "core/tool.hpp"
#include "util/keep_slope_util.hpp"
#include "tool_helper_plane.hpp"

namespace horizon {

class ToolDragPolygonEdge : public virtual ToolBase, public ToolHelperPlane {
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
        using I = InToolActionID;
        return {
                I::LMB,
                I::CANCEL,
                I::RMB,
        };
    }

private:
    class PolyInfo : public KeepSlopeInfo {
    public:
        PolyInfo(const class Polygon &poly, int edge);

        Coordi arc_center_orig;
    };
    std::optional<PolyInfo> poly_info;

    class Polygon *poly = nullptr;
    unsigned int edge = 0;
    Coordi pos_orig;
    void update_tip();
};
} // namespace horizon
