#pragma once
#include "util/uuid.hpp"
#include <nlohmann/json_fwd.hpp>
#include "util/uuid_ptr.hpp"

namespace horizon {
using json = nlohmann::json;

class BoardNetTie {
public:
    BoardNetTie(const UUID &uu, const json &j, class Board *board = nullptr);
    BoardNetTie(const UUID &uu);
    UUID uuid;
    UUID get_uuid() const;

    void update_refs(class Board &brd);

    uuid_ptr<class NetTie> net_tie;
    uuid_ptr<class BoardJunction> from;
    uuid_ptr<class BoardJunction> to;

    int layer = 0;
    uint64_t width = 0;
    bool width_from_rules = true;

    json serialize() const;
};
} // namespace horizon
