#pragma once
#include "common/polygon.hpp"
#include "clipper/clipper.hpp"
#include "nlohmann/json_fwd.hpp"
#include <vector>

namespace horizon {
using json = nlohmann::json;

class CopperThiefSettings {
public:
    CopperThiefSettings(const json &j);
    CopperThiefSettings()
    {
    }

    uint64_t size = 2.5_mm;
    uint64_t gap = 0.5_mm;

    json serialize() const;
};

std::vector<Coordi> get_thieving_pad_centers(const CopperThiefSettings &settings, const ClipperLib::Paths &area);
ClipperLib::Path get_thieving_pad_path(const CopperThiefSettings &settings, const Coordi &center);

class CopperThief : public PolygonUsage {
public:
    CopperThief(const UUID &uu, const json &j, class ObjectProvider *prv);
    CopperThief(const UUID &uu);
    UUID uuid;
    uuid_ptr<Polygon> polygon;
    CopperThiefSettings settings;
    std::vector<Coordi> pad_centers;
    unsigned int revision = 0;
    unsigned int get_revision() const
    {
        return revision;
    }
    void clear();
    void regenerate(const ClipperLib::Paths &area);
    json serialize_pads() const;
    void load_pads(const json &j);

    ObjectType get_type() const override;

    UUID get_uuid() const override
    {
        return uuid;
    }

    json serialize() const;
};

} // namespace horizon
