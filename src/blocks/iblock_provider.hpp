#pragma once

namespace horizon {
class IBlockProvider {
public:
    virtual class Block &get_block(const class UUID &uu) = 0;
};
} // namespace horizon
