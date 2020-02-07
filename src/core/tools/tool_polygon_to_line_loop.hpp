#pragma once
#include "core/tool.hpp"

namespace horizon {

class ToolPolygonToLineLoop : public ToolBase {
public:
    ToolPolygonToLineLoop(IDocument *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    bool is_specific() override
    {
        return true;
    }

private:
    class Polygon *get_polygon();
    class Junction *make_junction(const Coordi &c);
};
} // namespace horizon
