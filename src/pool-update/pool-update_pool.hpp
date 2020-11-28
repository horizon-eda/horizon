#pragma once
#include "pool/pool.hpp"

namespace horizon {
class PoolUpdatePool : public Pool {
public:
    using Pool::Pool;
    void inject_part(const class Part &part, const std::string &filename, const UUID &pool_uuid);
    const std::string &get_part_filename(const UUID &uu) const;

private:
    std::map<UUID, std::string> part_filenames;
};
} // namespace horizon
