#include "schematic_block_symbol.hpp"
#include "common/lut.hpp"
#include "nlohmann/json.hpp"
#include "schematic.hpp"
#include "blocks/iblock_symbol_and_schematic_provider.hpp"

namespace horizon {

SchematicBlockSymbol::SchematicBlockSymbol(const UUID &uu, const json &j, class IBlockSymbolAndSchematicProvider &prv,
                                           Block &block)
    : uuid(uu), block_instance(&block.block_instances.at(j.at("block_instance").get<std::string>())),
      prv_symbol(&prv.get_block_symbol(block_instance->block->uuid)), symbol(*prv_symbol),
      schematic(&prv.get_schematic(block_instance->block->uuid)), placement(j.at("placement"))
{
}

SchematicBlockSymbol::SchematicBlockSymbol(const UUID &uu, const BlockSymbol &sym, BlockInstance &inst)
    : uuid(uu), block_instance(&inst), prv_symbol(&sym), symbol(sym)
{
}

json SchematicBlockSymbol::serialize() const
{
    json j;
    j["block_instance"] = (std::string)block_instance->uuid;
    j["placement"] = placement.serialize();

    return j;
}

UUID SchematicBlockSymbol::get_uuid() const
{
    return uuid;
}

std::string SchematicBlockSymbol::replace_text(const std::string &t, bool *replaced, const Schematic &sch) const
{
    if (replaced)
        *replaced = false;
    if (block_instance) {
        return block_instance->replace_text(t, replaced);
    }
    return "";
}
} // namespace horizon
