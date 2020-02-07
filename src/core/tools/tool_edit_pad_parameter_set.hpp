#pragma once
#include "core/tool.hpp"

namespace horizon {

class ToolEditPadParameterSet : public ToolBase {
public:
    ToolEditPadParameterSet(IDocument *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    bool is_specific() override
    {
        return true;
    }

private:
    std::set<class Pad *> get_pads();
};
} // namespace horizon
