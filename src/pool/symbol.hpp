#pragma once
#include "common/arc.hpp"
#include "common/common.hpp"
#include "common/junction.hpp"
#include "common/layer_provider.hpp"
#include "common/line.hpp"
#include "common/object_provider.hpp"
#include "common/polygon.hpp"
#include "common/text.hpp"
#include "nlohmann/json_fwd.hpp"
#include "unit.hpp"
#include "util/uuid.hpp"
#include "symbol/symbol_rules.hpp"

namespace horizon {
using json = nlohmann::json;

class SymbolPin {
public:
    enum class ConnectorStyle { BOX, NONE, NC };

    SymbolPin(const UUID &uu, const json &j);
    SymbolPin(UUID uu);

    UUID uuid;
    Coord<int64_t> position;
    uint64_t length = 2.5_mm;
    bool name_visible = true;
    bool pad_visible = true;

    enum class NameOrientation { IN_LINE, PERPENDICULAR, HORIZONTAL };
    NameOrientation name_orientation = NameOrientation::IN_LINE;

    Orientation orientation = Orientation::RIGHT;
    Orientation get_orientation_for_placement(const Placement &p) const;

    class Decoration {
    public:
        Decoration();
        Decoration(const json &j);
        bool dot = false;
        bool clock = false;
        bool schmitt = false;
        enum class Driver {
            DEFAULT,
            OPEN_COLLECTOR,
            OPEN_COLLECTOR_PULLUP,
            OPEN_EMITTER,
            OPEN_EMITTER_PULLDOWN,
            TRISTATE
        };
        Driver driver = Driver::DEFAULT;

        json serialize() const;
    };
    Decoration decoration;

    // not stored
    std::string name;
    std::string pad;
    ConnectorStyle connector_style = ConnectorStyle::BOX;
    std::map<UUID, class LineNet *> connected_net_lines;
    UUID net_segment;
    Pin::Direction direction = Pin::Direction::BIDIRECTIONAL;

    json serialize() const;
    UUID get_uuid() const;
};

class Symbol : public ObjectProvider, public LayerProvider {
public:
    Symbol(const UUID &uu, const json &j, class IPool &pool);
    Symbol(const UUID &uu);
    static Symbol new_from_file(const std::string &filename, IPool &pool);
    std::pair<Coordi, Coordi> get_bbox(bool all = false) const;
    virtual Junction *get_junction(const UUID &uu) override;

    json serialize() const;

    /**
     * fills in information from the referenced unit
     */
    enum class PinDisplayMode { PRIMARY, ALT, BOTH };
    void expand(PinDisplayMode mode = PinDisplayMode::PRIMARY);
    Symbol(const Symbol &sym);
    void operator=(Symbol const &sym);

    UUID uuid;
    uuid_ptr<const Unit> unit;
    std::string name;
    std::map<UUID, SymbolPin> pins;
    std::map<UUID, Junction> junctions;
    std::map<UUID, Line> lines;
    std::map<UUID, Arc> arcs;
    std::map<UUID, Text> texts;
    std::map<UUID, Polygon> polygons;
    bool can_expand = false;

    std::map<std::tuple<int, bool, UUID>, Placement> text_placements;
    void apply_placement(const Placement &p);

    SymbolRules rules;

private:
    void update_refs();
};
} // namespace horizon
