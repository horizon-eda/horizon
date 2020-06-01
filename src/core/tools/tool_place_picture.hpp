#pragma once
#include "core/tool.hpp"

namespace horizon {
class ToolPlacePicture : public ToolBase {
public:
    ToolPlacePicture(IDocument *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;

private:
    class Picture *temp = nullptr;
};
} // namespace horizon
