#pragma once
#include "core.hpp"

namespace horizon {

class ToolLineLoopToPolygon : public ToolBase {
public:
    ToolLineLoopToPolygon(Core *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    bool is_specific() override
    {
        return true;
    }
};
} // namespace horizon
