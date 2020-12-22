#pragma once
#include "common/polygon.hpp"
#include "tool_helper_plane.hpp"

namespace horizon {

class ToolRoundOffVertex : public virtual ToolBase, public ToolHelperPlane {
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
                I::LMB, I::CANCEL, I::RMB, I::ENTER_DATUM, I::FLIP_ARC,
        };
    }

private:
    Polygon *poly = nullptr;
    int wrap_index(int i) const;

    Polygon::Vertex *vxn = nullptr;
    Polygon::Vertex *vxp = nullptr;

    Coord<double> p0;
    Coord<double> vp;
    Coord<double> vn;
    Coord<double> vh;
    double delta_max = 0;
    double r_max = 0;
    double alpha = 0;
    double radius_current = 0;

    void update_poly(double r);
    void update_cursor(const Coordi &c);
    void update_tip();

    bool orientation = false;
};
} // namespace horizon
