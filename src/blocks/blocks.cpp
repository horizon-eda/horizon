#include "blocks.hpp"
#include "nlohmann/json.hpp"
#include "logger/log_util.hpp"
#include "util/util.hpp"
#include "util/dependency_graph.hpp"
#include <filesystem>

namespace horizon {
namespace fs = std::filesystem;


BlocksBase::BlocksBase() : top_block(UUID::random())
{
}

std::map<std::string, std::string> BlocksBase::peek_project_meta(const std::string &filename)
{
    const auto j = load_json_from_file(filename);
    const auto top_block = j.at("top_block").get<std::string>();
    const auto block_filename = j.at("blocks").at(top_block).at("block_filename").get<std::string>();
    return Block::peek_project_meta((fs::u8path(filename).parent_path() / fs::u8path(block_filename)).u8string());
}


class BlocksDependencyGraph : public DependencyGraph {
public:
    using DependencyGraph::DependencyGraph;
    void add_block(const UUID &uu, const std::set<UUID> &blocks)
    {
        std::vector<UUID> deps;
        deps.insert(deps.begin(), blocks.begin(), blocks.end());
        nodes.emplace(std::piecewise_construct, std::forward_as_tuple(uu), std::forward_as_tuple(uu, deps));
    }


    void dump(const std::string &filename, BlocksBase &blocks) const
    {
        auto ofs = make_ofstream(filename);
        ofs << "digraph {\n";
        for (const auto &it : nodes) {
            const auto name = blocks.get_block(it.second.uuid).name;
            ofs << "\"" << (std::string)it.first << "\" [label=\"" << name << "\"]\n";
        }
        for (const auto &it : nodes) {
            for (const auto &dep : it.second.dependencies) {
                ofs << "\"" << (std::string)it.first << "\" -> \"" << (std::string)dep << "\"\n";
            }
        }
        ofs << "}";
    }


private:
};


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
    : base_path(bp), top_block(j.at("top_block").get<std::string>())
{
}

std::vector<BlocksBase::BlockItemInfo> BlocksBase::blocks_sorted_from_json(const json &j) const
{
    const auto blocks_info = blocks_info_from_json(j);

    BlocksDependencyGraph graph(top_block);

    for (const auto &[uu, it] : blocks_info) {
        const auto blocks_inst =
                Block::peek_instantiated_blocks((fs::u8path(base_path) / it.block_filename).u8string());
        graph.add_block(uu, blocks_inst);
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
      block(Block::new_from_file(fs::u8path(base_path) / fs::u8path(block_filename), pool, blocks))
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
    return j;
}

BlocksBase::BlocksBase(const BlocksBase &other) : base_path(other.base_path), top_block(other.top_block)
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

BlocksBase::BlockItem &Blocks::get_top_block()
{
    return blocks.at(top_block);
}

const BlocksBase::BlockItem &Blocks::get_top_block() const
{
    return blocks.at(top_block);
}

Block &Blocks::get_block(const UUID &uu)
{
    return blocks.at(uu).block;
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
                     Logger::Domain::BLOCK);
    }
}

} // namespace horizon
