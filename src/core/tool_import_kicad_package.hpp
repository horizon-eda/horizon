#pragma once
#include "core.hpp"

namespace horizon {

class ToolImportKiCadPackage : public ToolBase {
public:
    ToolImportKiCadPackage(Core *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args);
    ToolResponse update(const ToolArgs &args);
    bool can_begin();

private:
};
} // namespace horizon
