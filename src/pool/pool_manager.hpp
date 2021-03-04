#pragma once
#include "util/uuid.hpp"
#include <map>
#include <sigc++/sigc++.h>
#include <vector>
#include "pool_info.hpp"

namespace horizon {

class PoolManagerPool : public PoolInfo {
public:
    using PoolInfo::PoolInfo;
    bool enabled = false;
};

class PoolManager {
public:
    PoolManager();
    static PoolManager &get();
    static void init();
    std::string get_pool_base_path(const UUID &uu);
    void set_pool_enabled(const std::string &base_path, bool enabled);
    bool get_pool_enabled(const std::string &base_path) const;
    void add_pool(const std::string &base_path);
    void remove_pool(const std::string &base_path);
    void update_pool(const std::string &base_path);
    const std::map<std::string, PoolManagerPool> &get_pools() const;
    const PoolManagerPool *get_by_uuid(const UUID &uu) const;

private:
    std::map<std::string, PoolManagerPool> pools;
    void set_pool_enabled_no_write(const std::string &base_path, bool enabled);
    void write();
};
} // namespace horizon
