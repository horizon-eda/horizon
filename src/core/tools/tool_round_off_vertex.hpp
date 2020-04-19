#pragma once
#include "common/polygon.hpp"
#include "core/tool.hpp"
#include <forward_list>

namespace horizon {

class ToolRoundOffVertex : public ToolBase {
public:
    ToolRoundOffVertex(IDocument *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    bool is_specific() override
    {
        return true;
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
