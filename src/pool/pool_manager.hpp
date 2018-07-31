#pragma once
#include "util/uuid.hpp"
#include <map>
#include <sigc++/sigc++.h>
#include <vector>

namespace horizon {

class PoolManagerPool {
public:
    PoolManagerPool(const std::string &bp);
    std::string base_path;
    UUID uuid;
    UUID default_via;
    bool enabled = false;
    std::string name;
    std::vector<UUID> pools_included;
    void save();
};

class PoolManager : public sigc::trackable {
public:
    PoolManager();
    static PoolManager &get();
    static void init();
    std::string get_pool_base_path(const UUID &uu);
    void set_pool_enabled(const std::string &base_path, bool enabled);
    bool get_pool_enabled(const std::string &base_path) const;
    void add_pool(const std::string &base_path);
    void remove_pool(const std::string &base_path);
    void update_pool(const std::string &base_path, const PoolManagerPool &settings);
    const std::map<std::string, PoolManagerPool> &get_pools() const;
    const PoolManagerPool *get_by_uuid(const UUID &uu) const;

    typedef sigc::signal<void> type_signal_changed;
    type_signal_changed signal_changed()
    {
        return s_signal_changed;
    }

private:
    std::map<std::string, PoolManagerPool> pools;
    void set_pool_enabled_no_write(const std::string &base_path, bool enabled);
    void write();

    type_signal_changed s_signal_changed;
};
} // namespace horizon
