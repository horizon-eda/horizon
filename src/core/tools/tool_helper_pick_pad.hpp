#pragma once
#include "core/tool.hpp"
#include <optional>

namespace horizon {
class ToolHelperPickPad : public virtual ToolBase {
protected:
    struct PkgPad {
        class BoardPackage &pkg;
        class Pad &pad;
    };
    std::optional<PkgPad> pad_from_target(const Target &target);
};
} // namespace horizon
