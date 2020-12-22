#pragma once
#include <set>
#include "canvas/selectables.hpp"
#include "core/tool.hpp"

namespace horizon {
class ToolHelperCollectNets : public virtual ToolBase {
protected:
    std::set<UUID> nets_from_selection(const std::set<SelectableRef> &sel);
};
} // namespace horizon
