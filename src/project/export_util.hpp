#pragma once
#include "block/block.hpp"

namespace horizon {
// Prepare the schematic for export, including the symbols and sheets in child blocks
void expand_schematic_for_export(class BlocksSchematic &blocks);
// Load the blocks and flatten their netlist so the board exporters can work with the whole project
Block load_flattened_block(const std::string &filename, class IPool &pool);
} // namespace horizon
