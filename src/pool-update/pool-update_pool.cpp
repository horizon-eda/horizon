#include "pool-update_pool.hpp"
#include "pool/part.hpp"

namespace horizon {
void PoolUpdatePool::inject_part(const class Part &part, const std::string &filename, const UUID &pool_uuid)
{
    if (parts.count(part.uuid))
        return;
    parts.emplace(part.uuid, part);
    pool_uuid_cache.emplace(std::piecewise_construct, std::forward_as_tuple(ObjectType::PART, part.uuid),
                            std::forward_as_tuple(pool_uuid));
    part_filenames.emplace(part.uuid, filename);
}

const std::string &PoolUpdatePool::get_part_filename(const UUID &uu) const
{
    return part_filenames.at(uu);
}
} // namespace horizon
