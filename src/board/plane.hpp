#pragma once
#include "block/net.hpp"
#include "clipper/clipper.hpp"
#include "common/polygon.hpp"

namespace horizon {
using json = nlohmann::json;

class ThermalSettings {
public:
    ThermalSettings()
    {
    }

    ThermalSettings(const json &j);

    enum class ConnectStyle { SOLID, THERMAL, FROM_PLANE };
    ConnectStyle connect_style = ConnectStyle::SOLID;

    uint64_t thermal_gap_width = 0.2_mm;
    uint64_t thermal_spoke_width = 0.2_mm;
    unsigned int n_spokes = 4;
    int angle = 0;

    void serialize(json &j) const;
};

class PlaneSettings {
public:
    PlaneSettings(const json &j);
    PlaneSettings()
    {
    }
    enum class Style { ROUND, SQUARE, MITER };
    uint64_t min_width = 0.2_mm;
    Style style = Style::ROUND;
    uint64_t extra_clearance = 0;
    bool keep_orphans = false;

    ThermalSettings thermal_settings;

    enum class TextStyle { EXPAND, BBOX };
    TextStyle text_style = TextStyle::EXPAND;

    enum class FillStyle { SOLID, HATCH };
    FillStyle fill_style = FillStyle::SOLID;
    uint64_t hatch_border_width = 0.5_mm;
    uint64_t hatch_line_width = 0.2_mm;
    uint64_t hatch_line_spacing = 0.5_mm;

    json serialize() const;
};

class Plane : public PolygonUsage {
public:
    class Fragment {
    public:
        Fragment()
        {
        }
        Fragment(const json &j);
        bool orphan = false;
        ClipperLib::Paths paths;              // first path is outline, others are holes
        bool contains(const Coordi &c) const; // checks if point is in area defined by paths
        json serialize() const;
    };

    Plane(const UUID &uu, const json &j, class Board *brd);
    Plane(const UUID &uu);
    UUID uuid;
    uuid_ptr<Net> net;
    uuid_ptr<Polygon> polygon;
    bool from_rules = true;
    int priority = 0;
    PlaneSettings settings;

    std::deque<Fragment> fragments;
    void clear();
    unsigned int get_revision() const
    {
        return revision;
    }
    void load_fragments(const json &j);

    Type get_type() const override;
    UUID get_uuid() const override;
    std::string get_name() const;

    json serialize() const;
    json serialize_fragments() const;

private:
    unsigned int revision = 0;
};
} // namespace horizon
