#pragma once
#include "common/polygon.hpp"
#include "core/tool.hpp"
#include "util/keep_slope_util.hpp"

namespace horizon {

class ToolDragPolygonEdge : public ToolBase {
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
        PolyInfo(const Polygon &poly, int edge);
    };
    std::optional<PolyInfo> poly_info;

    Polygon *poly = nullptr;
    class Plane *plane = nullptr;
    unsigned int edge = 0;
    Coordi pos_orig;
    void update_tip();
};
} // namespace horizon
