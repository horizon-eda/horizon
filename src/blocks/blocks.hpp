#pragma once
#include <nlohmann/json_fwd.hpp>
#include "block/block.hpp"
#include "iblock_provider.hpp"
#include "util/file_version.hpp"

namespace horizon {
using json = nlohmann::json;


class BlocksBase {
public:
    class BlockItemInfo {
    public:
        BlockItemInfo(const UUID &uu, const json &j);
        UUID uuid;
        std::string block_filename;
        std::string symbol_filename;
        std::string schematic_filename;

        json serialize() const;

    protected:
        BlockItemInfo(const UUID &uu, const std::string &b, const std::string &y, const std::string &c);
    };

    class BlockItem : public BlockItemInfo {
    public:
        BlockItem(const BlockItemInfo &inf, const std::string &base_path, IPool &pool, class IBlockProvider &blocks);

        // so we can use load_and_log
        BlockItem(const UUID &uu, const BlockItemInfo &inf, const std::string &base_path, IPool &pool,
                  class IBlockProvider &blocks);

        void update_refs(IBlockProvider &blocks);

        Block block;

    protected:
        BlockItem(const UUID &uu, const std::string &b, const std::string &y, const std::string &c);
        BlockItem(const BlockItemInfo &inf, const json &j, IPool &pool, class IBlockProvider &blocks);
    };

    static std::map<std::string, std::string> peek_project_meta(const std::string &filename);
    static std::vector<std::string> peek_filenames(const std::string &filename);

    std::string base_path;
    UUID top_block;

    FileVersion version;
    static unsigned int get_app_version();

protected:
    BlocksBase();
    BlocksBase(const BlocksBase &other);
    BlocksBase(const json &j, const std::string &base_path);

    std::vector<BlockItemInfo> blocks_sorted_from_json(const json &j) const;
    json serialize_base() const;
};

class Blocks : public BlocksBase, public IBlockProvider {
public:
    Blocks(const json &j, const std::string &base_path, IPool &pool);
    static Blocks new_from_file(const std::string &filename, IPool &pool);
    Blocks(const Blocks &other);

    std::map<UUID, BlockItem> blocks;

    BlockItem &get_top_block_item();
    const BlockItem &get_top_block_item() const;

    Block &get_block(const UUID &uu) override;
    std::map<UUID, Block *> get_blocks() override;
    Block &get_top_block() override;
    json serialize() const;
};

} // namespace horizon
