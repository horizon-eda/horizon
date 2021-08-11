#pragma once
#include "util/uuid.hpp"

namespace horizon {
class IBlockProvider {
public:
    virtual class Block &get_block(const UUID &uu) = 0;
    virtual std::map<UUID, Block *> get_blocks() = 0;
    virtual class Block &get_top_block() = 0;
};
} // namespace horizon
