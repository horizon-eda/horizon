#pragma once
#include "track.hpp"
#include "common/common.hpp"
#include "common/junction.hpp"
#include "nlohmann/json_fwd.hpp"
#include "util/uuid.hpp"
#include "util/uuid_ptr.hpp"

namespace horizon {
using json = nlohmann::json;

class ConnectionLine {
public:
    ConnectionLine(const UUID &uu, const json &j, class Board *brd = nullptr);
    ConnectionLine(const UUID &uu);

    void update_refs(class Board &brd);
    UUID get_uuid() const;

    UUID uuid;

    Track::Connection from;
    Track::Connection to;

    json serialize() const;
};
} // namespace horizon
