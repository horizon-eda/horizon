#pragma once
#include "util/uuid.hpp"
#include "util/placement.hpp"
#include "nlohmann/json_fwd.hpp"
#include "util/picture_data.hpp"

namespace horizon {
using json = nlohmann::json;

class Picture {
public:
    Picture(const UUID &uu, const json &j);
    Picture(const UUID &uu);

    UUID uuid;
    Placement placement;
    bool on_top = false;
    float opacity = 1;
    bool show_border = false;
    bool get_show_border() const;

    static constexpr float min_opacity = 1e-3;

    uint64_t px_size = 1;
    std::shared_ptr<const PictureData> data;
    UUID data_uuid;

    json serialize() const;
};

} // namespace horizon
