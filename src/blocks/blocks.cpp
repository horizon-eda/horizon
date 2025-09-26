#include "blocks.hpp"
#include <nlohmann/json.hpp>
#include "logger/log_util.hpp"
#include "util/util.hpp"
#include "dependency_graph.hpp"
#include "iblock_provider.hpp"
#include <filesystem>

namespace horizon {
namespace fs = std::filesystem;


static const unsigned int app_version = 0;

unsigned int BlocksBase::get_app_version()
{
    return app_version;
}

BlocksBase::BlocksBase() : top_block(UUID::random()), version(app_version)
{
}

std::map<std::string, std::string> BlocksBase::peek_project_meta(const std::string &filename)
{
    const auto j = load_json_from_file(filename);
    const auto top_block = j.at("top_block").get<std::string>();
    const auto block_filename = j.at("blocks").at(top_block).at("block_filename").get<std::string>();
    return Block::peek_project_meta((fs::u8path(filename).parent_path() / fs::u8path(block_filename)).u8string());
}

static std::map<UUID, BlocksBase::BlockItemInfo> blocks_info_from_json(const json &j)
{
    std::map<UUID, BlocksBase::BlockItemInfo> r;
    for (const auto &[k, it] : j.at("blocks").items()) {
        const UUID u(k);
        r.emplace(std::piecewise_construct, std::forward_as_tuple(u), std::forward_as_tuple(u, it));
    }
    return r;
}

BlocksBase::BlocksBase(const json &j, const std::string &bp)
    : base_path(bp), top_block(j.at("top_block").get<std::string>()), version(app_version, j)
{
}

std::vector<BlocksBase::BlockItemInfo> BlocksBase::blocks_sorted_from_json(const json &j) const
{
    const auto blocks_info = blocks_info_from_json(j);

    BlocksDependencyGraph graph(top_block);

    for (const auto &[uu, it] : blocks_info) {
        const auto block_filename = (fs::u8path(base_path) / fs::u8path(it.block_filename)).u8string();
        try {
            const auto blocks_inst = Block::peek_instantiated_blocks(block_filename);
            graph.add_block(uu, blocks_inst);
        }
        catch (const std::exception &e) {
            Logger::log_critical("error peeking instantiated blocks from" + block_filename, Logger::Domain::BLOCKS,
                                 e.what());
        }
        catch (...) {
            Logger::log_critical("error peeking instantiated blocks from" + block_filename, Logger::Domain::BLOCKS);
        }
    }

    std::vector<BlockItemInfo> out;
    for (const auto &uu : graph.get_sorted()) {
        const auto &block = blocks_info.at(uu);
        out.push_back(block);
    }
    return out;
}
BlocksBase::BlockItemInfo::BlockItemInfo(const UUID &uu, const json &j)
    : uuid(uu), block_filename(j.at("block_filename").get<std::string>()),
      symbol_filename(j.at("symbol_filename").get<std::string>()),
      schematic_filename(j.at("schematic_filename").get<std::string>())
{
}

BlocksBase::BlockItemInfo::BlockItemInfo(const UUID &uu, const std::string &b, const std::string &y,
                                         const std::string &c)
    : uuid(uu), block_filename(b), symbol_filename(y), schematic_filename(c)
{
}

BlocksBase::BlockItem::BlockItem(const UUID &uu, const BlocksBase::BlockItemInfo &inf, const std::string &base_path,
                                 IPool &pool, IBlockProvider &blocks)
    : BlocksBase::BlockItem(inf, base_path, pool, blocks)
{
}

BlocksBase::BlockItem::BlockItem(const BlocksBase::BlockItemInfo &inf, const std::string &base_path, IPool &pool,
                                 IBlockProvider &blocks)
    : BlocksBase::BlockItemInfo(inf),
      block(Block::new_from_file((fs::u8path(base_path) / fs::u8path(block_filename)).u8string(), pool, blocks))
{
    if (uuid != block.uuid)
        Logger::log_critical("Block UUID mismatch", Logger::Domain::BLOCKS,
                             "blocks=" + (std::string)uuid + " block=" + (std::string)block.uuid);
}

BlocksBase::BlockItem::BlockItem(const BlockItemInfo &inf, const json &j, IPool &pool, class IBlockProvider &blocks)
    : BlocksBase::BlockItemInfo(inf), block(j.at("uuid").get<std::string>(), j, pool, blocks)
{
}

BlocksBase::BlockItem::BlockItem(const UUID &uu, const std::string &b, const std::string &y, const std::string &c)
    : BlocksBase::BlockItemInfo(uu, b, y, c), block(uu)
{
}

json BlocksBase::BlockItemInfo::serialize() const
{
    json j;
    j["block_filename"] = block_filename;
    j["symbol_filename"] = symbol_filename;
    j["schematic_filename"] = schematic_filename;
    return j;
}

json BlocksBase::serialize_base() const
{
    json j;
    j["type"] = "blocks";
    j["top_block"] = (std::string)top_block;
    j["blocks"] = json::object();
    version.serialize(j);
    return j;
}

BlocksBase::BlocksBase(const BlocksBase &other)
    : base_path(other.base_path), top_block(other.top_block), version(other.version)
{
}

json Blocks::serialize() const
{
    json j = serialize_base();
    for (const auto &[uu, it] : blocks) {
        j["blocks"][(std::string)uu] = it.serialize();
    }
    return j;
}

void BlocksBase::BlockItem::update_refs(IBlockProvider &blocks)
{
    for (auto &[uu_i, inst] : block.block_instances) {
        inst.block = &blocks.get_block(inst.block.uuid);
    }
}

Blocks::Blocks(const Blocks &other) : BlocksBase(other), blocks(other.blocks)
{
    for (auto &[uu, it] : blocks) {
        it.update_refs(*this);
    }
}

BlocksBase::BlockItem &Blocks::get_top_block_item()
{
    return blocks.at(top_block);
}

const BlocksBase::BlockItem &Blocks::get_top_block_item() const
{
    return blocks.at(top_block);
}

Block &Blocks::get_block(const UUID &uu)
{
    return blocks.at(uu).block;
}

Block &Blocks::get_top_block()
{
    return get_top_block_item().block;
}

std::map<UUID, Block *> Blocks::get_blocks()
{
    std::map<UUID, Block *> r;
    for (auto &[uu, block] : blocks) {
        r.emplace(uu, &block.block);
    }
    return r;
}

Blocks Blocks::new_from_file(const std::string &filename, IPool &pool)
{
    const auto j = load_json_from_file(filename);
    return Blocks(j, fs::u8path(filename).parent_path().u8string(), pool);
}

Blocks::Blocks(const json &j, const std::string &bp, IPool &pool) : BlocksBase(j, bp)
{
    for (const auto &block : blocks_sorted_from_json(j)) {
        load_and_log(blocks, ObjectType::BLOCK, std::forward_as_tuple(block.uuid, block, base_path, pool, *this),
                     Logger::Domain::BLOCKS);
    }
}

class BlocksPeek : public BlocksBase {
public:
    BlocksPeek(const json &j, const std::string &bp) : BlocksBase(j, bp)
    {
        const auto base = fs::u8path(base_path);
        for (const auto &block : blocks_sorted_from_json(j)) {
            for (const auto &fn : {block.symbol_filename, block.block_filename, block.schematic_filename}) {
                if (fn.size()) {
                    filenames.push_back((base / fs::u8path(fn)).u8string());
                }
            }
        }
    }

    BlocksPeek(const std::string &filename)
        : BlocksPeek(load_json_from_file(filename), fs::u8path(filename).parent_path().u8string())
    {
    }

    std::vector<std::string> filenames;
};

std::vector<std::string> BlocksBase::peek_filenames(const std::string &filename)
{
    BlocksPeek blocks{filename};
    return blocks.filenames;
}


} // namespace horizon
