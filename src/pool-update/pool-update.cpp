#include "pool-update.hpp"
#include <nlohmann/json.hpp>
#include "pool/pool_manager.hpp"
#include "pool_updater.hpp"
#include "pool/pool_info.hpp"
#include "logger/logger.hpp"
#include "util/util.hpp"
#include <filesystem>

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

    std::set<UUID> parts_updated;
    if (filenames.size() == 0) {
        updater.update();
    }
    else {
        try {
            updater.update_some(filenames, parts_updated);
        }
        catch (const PoolUpdater::CompletePoolUpdateRequiredException &e) {
            updater.update();
        }
    }

    if (parametric) {
        if (!updater.was_partial_update()) // complete update
            pool_update_parametric(updater.get_pool(), status_cb);
        else if (parts_updated.size())
            pool_update_parametric(updater.get_pool(), status_cb, parts_updated);
    }

    {
        SQLite::Query q(updater.get_pool().get_db(), "UPDATE last_updated SET time = ?");
        q.bind_int64(1, std::filesystem::file_time_type::clock::now().time_since_epoch().count());
        q.step();
    }

    status_cb(PoolUpdateStatus::INFO, "", "Done");
    status_cb(PoolUpdateStatus::DONE, "", "");
}

} // namespace horizon
