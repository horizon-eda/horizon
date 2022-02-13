#pragma once
#include "common/junction.hpp"
#include "util/uuid_ptr.hpp"

namespace horizon {
class SchematicJunction : public Junction {
public:
    using Junction::Junction;

    uuid_ptr<class Net> net = nullptr;
    uuid_ptr<class Bus> bus = nullptr;
    UUID net_segment = UUID();

    std::vector<UUID> connected_net_lines;
    std::vector<UUID> connected_net_labels;
    std::vector<UUID> connected_bus_labels;
    std::vector<UUID> connected_bus_rippers;
    std::vector<UUID> connected_power_symbols;
    std::vector<UUID> connected_net_ties;
    bool only_lines_arcs_connected() const override;
    bool only_net_lines_connected() const;
};
} // namespace horizon
