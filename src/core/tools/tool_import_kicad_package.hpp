#pragma once
#include "core/tool.hpp"

namespace horizon {

class ToolImportKiCadPackage : public ToolBase {
public:
    ToolImportKiCadPackage(IDocument *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;

private:
};
} // namespace horizon
