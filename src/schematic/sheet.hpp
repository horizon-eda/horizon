#pragma once
#include "util/uuid.hpp"
#include "nlohmann/json_fwd.hpp"
#include "pool/unit.hpp"
#include "block/block.hpp"
#include "schematic_symbol.hpp"
#include "line_net.hpp"
#include "common/text.hpp"
#include "net_label.hpp"
#include "bus_label.hpp"
#include "bus_ripper.hpp"
#include "power_symbol.hpp"
#include "common/line.hpp"
#include "common/arc.hpp"
#include "util/warning.hpp"
#include "common/layer_provider.hpp"
#include "frame/frame.hpp"
#include "common/picture.hpp"
#include <vector>
#include <map>
#include <fstream>

namespace horizon {
using json = nlohmann::json;

class NetSegmentInfo {
public:
    NetSegmentInfo(LineNet *li);
    NetSegmentInfo(Junction *ju);
    bool has_label = false;
    bool has_power_sym = false;
    Coordi position;
    Net *net = nullptr;
    Bus *bus = nullptr;
    bool is_bus() const;
};

class Sheet : public ObjectProvider, public LayerProvider {
public:
    Sheet(const UUID &uu, const json &, Block &Block, class Pool &pool);
    Sheet(const UUID &uu);
    UUID uuid;
    std::string name;
    unsigned int index;

    std::map<UUID, Junction> junctions;
    std::map<UUID, SchematicSymbol> symbols;
    // std::map<UUID, class JunctionPin> junction_pins;
    std::map<UUID, class LineNet> net_lines;
    std::map<UUID, class Text> texts;
    std::map<UUID, NetLabel> net_labels;
    std::map<UUID, PowerSymbol> power_symbols;
    std::map<UUID, BusLabel> bus_labels;
    std::map<UUID, BusRipper> bus_rippers;
    std::map<UUID, Line> lines;
    std::map<UUID, Arc> arcs;
    std::map<UUID, Picture> pictures;
    std::map<std::string, std::string> title_block_values;
    std::vector<Warning> warnings;

    LineNet *split_line_net(LineNet *it, Junction *ju);
    void merge_net_lines(LineNet *a, LineNet *b, Junction *ju);
    void expand_symbols(const class Schematic &sch);
    void expand_symbol(const UUID &sym_uuid, const Schematic &sch);
    void simplify_net_lines(bool simplify);
    void fix_junctions();
    void delete_duplicate_net_lines();
    void vacuum_junctions();
    void delete_dependants();
    void propagate_net_segments();
    std::map<UUID, NetSegmentInfo> analyze_net_segments(bool place_warnings = false);
    std::set<UUIDPath<3>> get_pins_connected_to_net_segment(const UUID &uu_segment);

    void replace_junction(Junction *j, SchematicSymbol *sym, SymbolPin *pin);
    Junction *replace_bus_ripper(BusRipper *rip);

    void merge_junction(Junction *j, Junction *into); // merge junction j into "into" j will be deleted

    // void replace_junction(Junction *j, PowerSymbol *sym);
    // void replace_power_symbol(PowerSymbol *sym, Junction *j);
    // void connect(SchematicSymbol *sym, SymbolPin *pin, PowerSymbol
    // *power_sym);

    Junction *get_junction(const UUID &uu) override;

    uuid_ptr<const Frame> pool_frame;
    Frame frame;

    json serialize() const;

private:
    void expand_symbol_without_net_lines(const UUID &sym_uuid, const Schematic &sch);
};
} // namespace horizon
