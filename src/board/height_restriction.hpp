#pragma once
#include "common/polygon.hpp"

namespace horizon {
using json = nlohmann::json;

class HeightRestriction : public PolygonUsage {
public:
    HeightRestriction(const UUID &uu, const json &j, class ObjectProvider &prv);
    HeightRestriction(const UUID &uu);
    UUID uuid;
    uuid_ptr<Polygon> polygon;

    int64_t height = 0;

    ObjectType get_type() const override;
    UUID get_uuid() const override;

    json serialize() const;
};

} // namespace horizon
