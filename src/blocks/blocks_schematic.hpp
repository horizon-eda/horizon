#pragma once
#include "blocks.hpp"
#include "block_symbol/block_symbol.hpp"
#include "schematic/schematic.hpp"
#include "iblock_symbol_and_schematic_provider.hpp"

namespace horizon {
class BlocksSchematic : public BlocksBase, public IBlockSymbolAndSchematicProvider {
public:
    class BlockItemSchematic : public BlockItem {
    public:
        BlockItemSchematic(const BlockItemInfo &inf, const std::string &base_path, IPool &pool,
                           class BlocksSchematic &blocks);
        BlockItemSchematic(const UUID &uu, const std::string &block_name);

        // so we can use load_and_log
        BlockItemSchematic(const UUID &uu, const BlockItemInfo &inf, const std::string &base_path, IPool &pool,
                           class BlocksSchematic &blocks);

        BlockSymbol symbol;
        Schematic schematic;
    };

    BlocksSchematic();
    BlocksSchematic(const json &j, const std::string &base_path, IPool &pool);
    static BlocksSchematic new_from_file(const std::string &filename, IPool &pool);
    BlocksSchematic(const BlocksSchematic &other);

    std::map<UUID, BlockItemSchematic> blocks;

    BlockItemSchematic &get_top_block();
    const BlockItemSchematic &get_top_block() const;

    Block &get_block(const UUID &uu) override;
    BlockSymbol &get_block_symbol(const UUID &uu) override;
    Schematic &get_schematic(const UUID &uu) override;
    json serialize() const;
};

} // namespace horizon
