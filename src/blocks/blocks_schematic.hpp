#pragma once
#include "blocks.hpp"
#include "block_symbol/block_symbol.hpp"
#include "schematic/schematic.hpp"
#include "iblock_symbol_and_schematic_provider.hpp"

namespace horizon {
class BlocksSchematic : public BlocksBase, public IBlockProvider, public IBlockSymbolAndSchematicProvider {
public:
    class BlockItemSchematic : public BlockItem {
    public:
        BlockItemSchematic(const BlockItemInfo &inf, const std::string &base_path, IPool &pool,
                           class BlocksSchematic &blocks);
        BlockItemSchematic(const UUID &uu, const std::string &block_name);

        // so we can use load_and_log
        BlockItemSchematic(const UUID &uu, const BlockItemInfo &inf, const std::string &base_path, IPool &pool,
                           class BlocksSchematic &blocks);

        BlockItemSchematic(const BlockItemInfo &inf, const json &j_block, const json &j_sym, const json &j_sch,
                           IPool &pool, BlocksSchematic &blocks);

        BlockSymbol symbol;
        Schematic schematic;
    };

    BlocksSchematic();
    BlocksSchematic(const json &j, const std::string &base_path, IPool &pool);
    static BlocksSchematic new_from_file(const std::string &filename, IPool &pool);
    BlocksSchematic(const BlocksSchematic &other);

    std::map<UUID, BlockItemSchematic> blocks;

    std::vector<const BlockItemSchematic *> get_blocks_sorted() const;

    BlockItemSchematic &get_top_block_item();
    const BlockItemSchematic &get_top_block_item() const;

    BlockItemSchematic &add_block(const std::string &name);

    Block &get_block(const UUID &uu) override;
    std::map<UUID, Block *> get_blocks() override;
    Block &get_top_block() override;
    BlockSymbol &get_block_symbol(const UUID &uu) override;
    Schematic &get_schematic(const UUID &uu) override;
    json serialize() const;
};

} // namespace horizon
