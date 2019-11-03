#pragma once
#include "util/uuid.hpp"
#include "nlohmann/json_fwd.hpp"
#include "pool/unit.hpp"
#include "pool/symbol.hpp"
#include "pool/gate.hpp"
#include "block/block.hpp"
#include "util/uuid_ptr.hpp"
#include "util/placement.hpp"
#include "util/uuid_provider.hpp"
#include "pool/pool.hpp"
#include <vector>
#include <map>
#include <fstream>

namespace horizon {
using json = nlohmann::json;

class SchematicSymbol : public UUIDProvider {
public:
    SchematicSymbol(const UUID &uu, const json &, Pool &pool, Block *block = nullptr);
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

    std::string replace_text(const std::string &t, bool *replaced, const class Schematic &sch) const;

    UUID get_uuid() const override;
    json serialize() const;
};
} // namespace horizon
