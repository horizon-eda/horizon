#pragma once
#include "core/tool.hpp"

namespace horizon {

class ToolRenumberPads : public ToolBase {
public:
    ToolRenumberPads(IDocument *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    bool is_specific() override
    {
        return true;
    }
    ~ToolRenumberPads();

private:
    std::set<UUID> get_pads();
    class RenumberPadsWindow *win = nullptr;
    class CanvasAnnotation *annotation = nullptr;
};
} // namespace horizon
