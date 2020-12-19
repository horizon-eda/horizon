#pragma once
#include "common/junction.hpp"
#include "util/uuid_ptr.hpp"

namespace horizon {
class BoardJunction : public Junction {
public:
    using Junction::Junction;

    uuid_ptr<class Net> net = nullptr;
    bool needs_via = false;
    bool has_via = false;
    std::vector<UUID> connected_vias;
    std::vector<UUID> connected_tracks;
    std::vector<UUID> connected_connection_lines;
};
} // namespace horizon
