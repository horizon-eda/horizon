#include "schematic_junction.hpp"

namespace horizon {
bool SchematicJunction::only_lines_arcs_connected() const
{
    return connected_net_lines.size() == 0 && connected_net_labels.size() == 0 && connected_bus_labels.size() == 0
           && connected_bus_rippers.size() == 0 && connected_power_symbols.size() == 0;
}

bool SchematicJunction::only_net_lines_connected() const
{
    return connected_lines.size() == 0 && connected_arcs.size() == 0 && connected_net_labels.size() == 0
           && connected_bus_labels.size() == 0 && connected_bus_rippers.size() == 0
           && connected_power_symbols.size() == 0;
}
} // namespace horizon
