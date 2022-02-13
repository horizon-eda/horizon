#include "board_junction.hpp"

namespace horizon {
bool BoardJunction::only_lines_arcs_connected() const
{
    return connected_vias.size() == 0 && connected_tracks.size() == 0 && connected_connection_lines.size() == 0
           && connected_net_ties.size() == 0;
}
} // namespace horizon
