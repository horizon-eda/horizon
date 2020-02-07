#pragma once
#include "core/tool.hpp"
#include <set>

namespace horizon {

class ToolGenerateCourtyard : public ToolBase {
public:
    ToolGenerateCourtyard(IDocument *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    bool is_specific() override
    {
        return false;
    }
};
} // namespace horizon
