#pragma once

namespace horizon {
class IBlockSymbolProvider {
public:
    virtual class BlockSymbol &get_block_symbol(const class UUID &uu) = 0;
};
} // namespace horizon
