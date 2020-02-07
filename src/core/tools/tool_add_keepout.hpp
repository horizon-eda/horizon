#pragma once
#include "core/tool.hpp"

namespace horizon {
class ToolAddKeepout : public ToolBase {
public:
    ToolAddKeepout(IDocument *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    bool is_specific() override
    {
        return true;
    }

private:
    class Polygon *get_poly();
};
} // namespace horizon
