#pragma once
#include "common/polygon.hpp"
#include "clipper/clipper.hpp"
#include "nlohmann/json_fwd.hpp"

namespace horizon {
using json = nlohmann::json;

class CopperThiefSettings {
public:
    CopperThiefSettings(const json &j);
    CopperThiefSettings()
    {
    }

    enum class Shape { ROUND, SQUARE };
    Shape shape = Shape::ROUND;
    uint64_t size = 2.5_mm;
    uint64_t gap = 0.5_mm;

    json serialize() const;
};

ClipperLib::Paths get_thieving_pads(const CopperThiefSettings &settings, const ClipperLib::Paths &area);

class CopperThief : public PolygonUsage {
public:
    class Fragment {
    public:
        Fragment() = default;
        Fragment(const json &j);
        bool orphan = false;
        ClipperLib::Paths paths;
        json serialize() const;
    };
    CopperThief(const UUID &uu, const json &j, class ObjectProvider *prv);
    CopperThief(const UUID &uu);
    UUID uuid;
    uuid_ptr<Polygon> polygon;
    CopperThiefSettings settings;
    std::deque<Fragment> fragments;
    unsigned int revision = 0;
    unsigned int get_revision() const
    {
        return revision;
    }
    void clear();
    void set_fragments(const ClipperLib::PolyTree &tree);
    void regenerate(const ClipperLib::Paths &area);
    json serialize_fragments() const;
    void load_fragments(const json &j);

    ObjectType get_type() const override;

    UUID get_uuid() const override
    {
        return uuid;
    }

    json serialize() const;
};

} // namespace horizon
