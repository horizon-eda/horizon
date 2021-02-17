#include "pool-update.hpp"
#include "nlohmann/json.hpp"
#include "pool/pool_manager.hpp"
#include "pool_updater.hpp"

namespace horizon {

using json = nlohmann::json;

static void status_cb_nop(PoolUpdateStatus st, const std::string msg, const std::string filename)
{
}

void pool_update(const std::string &pool_base_path, pool_update_cb_t status_cb, bool parametric,
                 const std::vector<std::string> &filenames)
{
    if (!status_cb)
        status_cb = &status_cb_nop;
    PoolUpdater updater(pool_base_path, status_cb);
    const auto pools = PoolManager::get().get_pools();
    std::vector<std::string> paths;
    if (pools.count(pool_base_path)) {
        const auto &pool_info = pools.at(pool_base_path);
        for (const auto &it : pool_info.pools_included) {
            auto inc = PoolManager::get().get_by_uuid(it);
            if (inc) {
                paths.push_back(inc->base_path);
            }
            else {
                status_cb(PoolUpdateStatus::FILE_ERROR, pool_base_path, "pool " + (std::string)it + " not found");
            }
        }
    }
    paths.push_back(pool_base_path);
    std::set<UUID> parts_updated;
    if (filenames.size() == 0) {
        updater.update(paths);
    }
    else {
        updater.update_some(pool_base_path, filenames, parts_updated);
    }

    if (parametric) {
        if (filenames.size() == 0) // complete update
            pool_update_parametric(updater.get_pool(), status_cb);
        else if (parts_updated.size())
            pool_update_parametric(updater.get_pool(), status_cb, parts_updated);
    }
    status_cb(PoolUpdateStatus::INFO, "", "Done");
    status_cb(PoolUpdateStatus::DONE, "", "");
}

} // namespace horizon
