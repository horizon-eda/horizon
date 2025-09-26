#pragma once
#include <nlohmann/json_fwd.hpp>
#include "pool/decal.hpp"
#include "util/placement.hpp"
#include "util/uuid.hpp"
#include "util/uuid_ptr.hpp"

namespace horizon {
using json = nlohmann::json;

class BoardDecal {
public:
    BoardDecal(const UUID &uu, const json &, class IPool &pool, const class Board &brd);
    BoardDecal(const UUID &uu, std::shared_ptr<const Decal> dec);
    UUID uuid;

    const LayerRange &get_layers() const;
    const Decal &get_decal() const;

    Placement placement;

    void set_flip(bool b, const class Board &brd);
    bool get_flip() const;

    void set_scale(double sc);
    double get_scale() const;

    UUID get_uuid() const;
    json serialize() const;

private:
    std::shared_ptr<const Decal> pool_decal;
    Decal decal;

    bool flip = false;
    double scale = 1;
    LayerRange layers;

    void update_layers();

    void apply_scale();
};
} // namespace horizon
