#include "blocks_schematic.hpp"
#include <filesystem>
#include <nlohmann/json.hpp>
#include "logger/log_util.hpp"
#include "util/util.hpp"
#include "dependency_graph.hpp"

namespace horizon {
namespace fs = std::filesystem;


BlocksSchematic::BlocksSchematic() : BlocksBase()
{
    auto &block = blocks.emplace(std::piecewise_construct, std::forward_as_tuple(top_block),
                                 std::forward_as_tuple(top_block, "Top"))
                          .first->second;
    block.block_filename = "top_block.json";
    block.schematic_filename = "top_schematic.json";
    block.symbol_filename.clear();
    block.symbol.uuid = UUID();
}

BlocksSchematic BlocksSchematic::new_from_file(const std::string &filename, IPool &pool)
{
    const auto j = load_json_from_file(filename);
    return BlocksSchematic(j, fs::u8path(filename).parent_path().u8string(), pool);
}

BlocksSchematic::BlocksSchematic(const json &j, const std::string &bp, IPool &pool) : BlocksBase(j, bp)
{
    for (const auto &block : blocks_sorted_from_json(j)) {
        load_and_log(blocks, ObjectType::BLOCK, std::forward_as_tuple(block.uuid, block, base_path, pool, *this),
                     Logger::Domain::BLOCKS);
    }
}

std::vector<const BlocksSchematic::BlockItemSchematic *> BlocksSchematic::get_blocks_sorted() const
{
    BlocksDependencyGraph graph(top_block);
    for (const auto &[uu, block] : blocks) {
        std::set<UUID> blocks_inst;
        for (const auto &[uu_inst, inst] : block.block.block_instances) {
            blocks_inst.insert(inst.block->uuid);
        }
        graph.add_block(uu, blocks_inst);
    }
    std::vector<const BlockItemSchematic *> sorted;
    for (const auto &uu : graph.get_sorted()) {
        sorted.push_back(&blocks.at(uu));
    }
    return sorted;
}


BlocksSchematic::BlockItemSchematic::BlockItemSchematic(const UUID &uu, const BlockItemInfo &inf,
                                                        const std::string &base_path, IPool &pool,
                                                        BlocksSchematic &blocks)
    : BlockItemSchematic(inf, base_path, pool, blocks)
{
}

BlocksSchematic::BlockItemSchematic::BlockItemSchematic(const BlockItemInfo &inf, const std::string &base_path,
                                                        IPool &pool, BlocksSchematic &blocks)
    : BlockItem(inf, base_path, pool, blocks),
      symbol(symbol_filename.size() ? BlockSymbol::new_from_file(
                                              (fs::u8path(base_path) / fs::u8path(symbol_filename)).u8string(), block)
                                    : BlockSymbol(UUID(), block)),
      schematic(Schematic::new_from_file((fs::u8path(base_path) / fs::u8path(schematic_filename)).u8string(), block,
                                         pool, blocks))
{
}

BlocksSchematic::BlockItemSchematic::BlockItemSchematic(const UUID &uu, const std::string &block_name)
    : BlockItem(uu, (fs::path("blocks") / ((std::string)uu) / "block.json").u8string(),
                (fs::path("blocks") / ((std::string)uu) / "symbol.json").u8string(),
                (fs::path("blocks") / ((std::string)uu) / "schematic.json").u8string()),
      symbol(UUID::random(), block), schematic(UUID::random(), block)
{
    block.name = block_name;
}

BlocksSchematic::BlockItemSchematic::BlockItemSchematic(const BlockItemInfo &inf, const json &j_block,
                                                        const json &j_sym, const json &j_sch, IPool &pool,
                                                        BlocksSchematic &blocks)
    : BlockItem(inf, j_block, pool, blocks), symbol(j_sym.at("uuid").get<std::string>(), j_sym, block),
      schematic(j_sch.at("uuid").get<std::string>(), j_sch, block, pool, blocks)
{
}

json BlocksSchematic::serialize() const
{
    json j = serialize_base();
    for (const auto &[uu, it] : blocks) {
        j["blocks"][(std::string)uu] = it.serialize();
    }
    return j;
}


BlocksSchematic::BlocksSchematic(const BlocksSchematic &other) : BlocksBase(other), blocks(other.blocks)
{
    for (auto &[uu, it] : blocks) {
        it.symbol.block = &it.block;
        it.symbol.update_refs();
        it.schematic.block = &it.block;
        it.schematic.update_refs();
        it.update_refs(*this);
        for (auto &[uu_sheet, sheet] : it.schematic.sheets) {
            for (auto &[uu_sym, sym] : sheet.block_symbols) {
                sym.prv_symbol = &get_block_symbol(sym.block_instance->block->uuid);
                sym.schematic = &get_schematic(sym.block_instance->block->uuid);
            }
        }
    }
}

BlocksSchematic::BlockItemSchematic &BlocksSchematic::add_block(const std::string &name)
{
    const auto uu = UUID::random();
    auto &b = blocks.emplace(std::piecewise_construct, std::forward_as_tuple(uu), std::forward_as_tuple(uu, name))
                      .first->second;
    b.block.net_classes.clear();
    const auto &top = get_top_block_item().block;
    auto &nc = b.block.net_classes.emplace(top.net_class_default->uuid, top.net_class_default->uuid).first->second;
    b.block.net_class_default = &nc;
    return b;
}

BlocksSchematic::BlockItemSchematic &BlocksSchematic::get_top_block_item()
{
    return blocks.at(top_block);
}

const BlocksSchematic::BlockItemSchematic &BlocksSchematic::get_top_block_item() const
{
    return blocks.at(top_block);
}

Block &BlocksSchematic::get_block(const UUID &uu)
{
    return blocks.at(uu).block;
}

Block &BlocksSchematic::get_top_block()
{
    return get_top_block_item().block;
}

std::map<UUID, Block *> BlocksSchematic::get_blocks()
{
    std::map<UUID, Block *> r;
    for (auto &[uu, block] : blocks) {
        r.emplace(uu, &block.block);
    }
    return r;
}

BlockSymbol &BlocksSchematic::get_block_symbol(const UUID &uu)
{
    return blocks.at(uu).symbol;
}

Schematic &BlocksSchematic::get_schematic(const UUID &uu)
{
    return blocks.at(uu).schematic;
}

} // namespace horizon
