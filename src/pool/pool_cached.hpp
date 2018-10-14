#pragma once
#include "pool.hpp"

namespace horizon {
class PoolCached : public Pool {
public:
    PoolCached(const std::string &base_path, const std::string &cache_path);
    const std::string &get_cache_path() const;
    std::string get_filename(ObjectType type, const UUID &uu, UUID *pool_uuid_out) override;
    std::string get_model_filename(const UUID &pkg_uuid, const UUID &model_uuid) override;


private:
    std::string cache_path;
};
} // namespace horizon
