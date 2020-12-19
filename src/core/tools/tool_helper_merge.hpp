#pragma once
#include "core/tool.hpp"

namespace horizon {
class ToolHelperMerge : public virtual ToolBase {
public:
    using ToolBase::ToolBase;

protected:
    bool merge_bus_net(class Net *net, class Bus *bus, class Net *net_other, class Bus *bus_other);
    int merge_nets(Net *net, Net *into);
    void merge_selected_junctions();

private:
};
} // namespace horizon
