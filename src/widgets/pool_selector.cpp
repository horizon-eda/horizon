#include "pool_selector.hpp"
#include "pool/ipool.hpp"
#include "pool/pool_info.hpp"
#include "util/sqlite.hpp"
#include "pool/pool_manager.hpp"

namespace horizon {
PoolSelector::PoolSelector(IPool &pool)
{
    append(UUID(), "All pools");
    set_active_key(UUID());
    {
        const auto &info = pool.get_pool_info();
        append(info.uuid, info.name + " (This)");
    }
    {
        SQLite::Query q(pool.get_db(), "SELECT uuid FROM pools_included WHERE level != 0 ORDER BY level ASC");
        while (q.step()) {
            const UUID uu = q.get<std::string>(0);
            if (auto pool2 = PoolManager::get().get_by_uuid(uu)) {
                append(uu, pool2->name);
            }
        }
    }
}

UUID PoolSelector::get_selected_pool() const
{
    return get_active_key();
}

} // namespace horizon
