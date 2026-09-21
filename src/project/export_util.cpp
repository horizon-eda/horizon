#include "export_util.hpp"
#include "blocks/blocks_schematic.hpp"

namespace horizon {
void expand_schematic_for_export(BlocksSchematic &blocks)
{
    auto &top = blocks.get_top_block_item();
    top.block.create_instance_mappings();
    top.schematic.update_sheet_mapping();
    for (auto &[uu, block] : blocks.blocks) {
        if (uu == top.uuid)
            continue;
        top.block.update_non_top(block.block);
    }
    for (auto &[uu, block] : blocks.blocks) {
        block.symbol.expand();
    }
    for (auto &[uu, block] : blocks.blocks) {
        block.schematic.expand();
    }
}

Block load_flattened_block(const std::string &filename, IPool &pool)
{
    auto blocks = Blocks::new_from_file(filename, pool);
    return blocks.get_top_block_item().block.flatten();
}
} // namespace horizon
