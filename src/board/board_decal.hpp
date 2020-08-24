#pragma once
#include "nlohmann/json_fwd.hpp"
#include "pool/decal.hpp"
#include "util/placement.hpp"
#include "util/uuid.hpp"
#include "util/uuid_ptr.hpp"

namespace horizon {
using json = nlohmann::json;

class BoardDecal {
public:
    BoardDecal(const UUID &uu, const json &, class IPool &pool);
    BoardDecal(const UUID &uu);
    UUID uuid;

    const Decal *pool_decal;
    Decal decal;

    Placement placement;
    bool flip = false;
    double scale = 1;

    void apply_scale();

    UUID get_uuid() const;
    json serialize() const;
};
} // namespace horizon
