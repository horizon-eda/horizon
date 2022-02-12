#pragma once
#include "util/uuid.hpp"
#include "nlohmann/json_fwd.hpp"
#include "pool/unit.hpp"
#include "pool/symbol.hpp"
#include "pool/gate.hpp"
#include "block/block.hpp"
#include "util/uuid_ptr.hpp"
#include "util/placement.hpp"
#include <vector>

namespace horizon {
using json = nlohmann::json;

class SchematicSymbol {
public:
    SchematicSymbol(const UUID &uu, const json &, class IPool &pool, Block *block = nullptr);
    SchematicSymbol(const UUID &uu, const Symbol *sym);
    UUID uuid;
    const Symbol *pool_symbol;
    Symbol symbol;
    uuid_ptr<Component> component;
    uuid_ptr<const Gate> gate;
    Placement placement;
    std::vector<uuid_ptr<Text>> texts;
    bool smashed = false;
    enum class PinDisplayMode { SELECTED_ONLY, BOTH, ALL, CUSTOM_ONLY };
    PinDisplayMode pin_display_mode = PinDisplayMode::SELECTED_ONLY;
    bool display_directions = false;
    bool display_all_pads = true;
    unsigned int expand = 0;
    void apply_expand();
    void apply_pin_names();

    std::string replace_text(const std::string &t, bool *replaced, const class Schematic &sch,
                             const class BlockInstanceMapping *inst_map = nullptr) const;
    std::string get_custom_value() const;

    std::string custom_value;

    UUID get_uuid() const;
    json serialize() const;
};
} // namespace horizon
