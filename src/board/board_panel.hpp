#pragma once
#include "included_board.hpp"

namespace horizon {
class BoardPanel {
public:
    BoardPanel(const UUID &uu, const json &j, const Board &brd);
    BoardPanel(const UUID &uu, const IncludedBoard &inc);

    json serialize() const;

    UUID uuid;
    uuid_ptr<const IncludedBoard> included_board;
    Placement placement;
    bool omit_outline = false;
};
} // namespace horizon
