#pragma once
#include "iblock_symbol_provider.hpp"
#include "iblock_schematic_provider.hpp"

namespace horizon {
class IBlockSymbolAndSchematicProvider : public IBlockSymbolProvider, public IBlockSchematicProvider {
};
} // namespace horizon
