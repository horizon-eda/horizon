#pragma once
#include "common/polygon.hpp"
#include "clipper/clipper.hpp"

namespace horizon {
using json = nlohmann::json;

class Keepout : public PolygonUsage {
public:
    Keepout(const UUID &uu, const json &j, class ObjectProvider &prv);
    Keepout(const UUID &uu);
    UUID uuid;
    uuid_ptr<Polygon> polygon;
    std::string keepout_class;

    std::set<PatchType> patch_types_cu;
    bool exposed_cu_only = false;
    bool all_cu_layers = false;

    Type get_type() const override;
    UUID get_uuid() const override;

    json serialize() const;
};

class KeepoutContour {
public:
    const Keepout *keepout = nullptr;
    const class BoardPackage *pkg = nullptr;
    ClipperLib::Path contour;
};

} // namespace horizon
