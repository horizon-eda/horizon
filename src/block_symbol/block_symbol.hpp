#pragma once
#include "common/arc.hpp"
#include "common/common.hpp"
#include "common/junction.hpp"
#include "common/layer_provider.hpp"
#include "common/line.hpp"
#include "common/object_provider.hpp"
#include "common/picture.hpp"
#include "common/text.hpp"
#include "nlohmann/json_fwd.hpp"
#include "util/uuid.hpp"
#include "util/placement.hpp"
#include "pool/unit.hpp"
#include "common/pin_name_orientation.hpp"
#include <map>

namespace horizon {
using json = nlohmann::json;

class BlockSymbolPort {
public:
    enum class ConnectorStyle { BOX, NONE, NC };

    BlockSymbolPort(const UUID &uu, const json &j);
    BlockSymbolPort(UUID uu);

    static UUID get_uuid_for_net(const UUID &net);

    UUID uuid;
    UUID net;
    Coord<int64_t> position;
    uint64_t length = 2.5_mm;

    Orientation orientation = Orientation::RIGHT;
    Orientation get_orientation_for_placement(const Placement &p) const;

    using NameOrientation = PinNameOrientation;
    NameOrientation name_orientation = NameOrientation::IN_LINE;

    // not stored
    std::string name;
    ConnectorStyle connector_style = ConnectorStyle::BOX;
    unsigned int connection_count = 0;
    UUID net_segment;
    Pin::Direction direction = Pin::Direction::BIDIRECTIONAL;

    json serialize() const;
    UUID get_uuid() const;
};

class BlockSymbol : public ObjectProvider, public LayerProvider {
public:
    BlockSymbol(const UUID &uu, const json &j, const class Block &iblock);
    BlockSymbol(const UUID &uu, const Block &iblock);
    static BlockSymbol new_from_file(const std::string &filename, const Block &iblock);
    std::pair<Coordi, Coordi> get_bbox(bool all = false) const;
    Junction *get_junction(const UUID &uu) override;

    BlockSymbolPort *get_port_for_net(const UUID &net);
    const BlockSymbolPort *get_port_for_net(const UUID &net) const;

    json serialize() const;

    void load_pictures(const std::string &dir);

    void create_template();

    /**
     * fills in information from the referenced block
     */
    void expand();
    BlockSymbol(const BlockSymbol &sym);
    void operator=(BlockSymbol const &sym);

    UUID uuid;
    const Block *block;
    std::map<UUID, BlockSymbolPort> ports;
    std::map<UUID, Junction> junctions;
    std::map<UUID, Line> lines;
    std::map<UUID, Arc> arcs;
    std::map<UUID, Text> texts;
    std::map<UUID, Picture> pictures;

    void update_refs();
};
} // namespace horizon
