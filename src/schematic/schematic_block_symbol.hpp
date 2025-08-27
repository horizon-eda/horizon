#pragma once
#include "util/uuid.hpp"
#include <nlohmann/json_fwd.hpp>
#include "block/block_instance.hpp"
#include "block_symbol/block_symbol.hpp"
#include "util/uuid_ptr.hpp"
#include "util/placement.hpp"

namespace horizon {
using json = nlohmann::json;

class SchematicBlockSymbol {
public:
    SchematicBlockSymbol(const UUID &uu, const json &, class IBlockSymbolAndSchematicProvider &prv, class Block &block);
    SchematicBlockSymbol(const UUID &uu, const BlockSymbol &sym, BlockInstance &inst);
    UUID uuid;
    uuid_ptr<BlockInstance> block_instance;
    const BlockSymbol *prv_symbol;
    BlockSymbol symbol;
    class Schematic *schematic = nullptr;
    Placement placement;

    static UUID peek_block_instance_uuid(const json &j);

    std::string replace_text(const std::string &t, bool *replaced, const class Schematic &sch) const;

    UUID get_uuid() const;
    json serialize() const;
};
} // namespace horizon
