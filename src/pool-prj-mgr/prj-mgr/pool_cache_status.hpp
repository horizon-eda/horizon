#pragma once
#include "common/common.hpp"
#include "util/uuid.hpp"
#include "pool_cache_status.hpp"
#include "nlohmann/json.hpp"
#include <list>

namespace horizon {
using json = nlohmann::json;

class PoolCacheStatus {
public:
    class Item {
    public:
        std::string name;
        std::string filename_cached;
        ObjectType type;
        UUID uuid;
        enum class State { CURRENT, OUT_OF_DATE, MISSING_IN_POOL };

        State state;
        json delta;
    };

    std::list<Item> items;
    unsigned int n_total = 0;
    unsigned int n_current = 0;
    unsigned int n_missing = 0;
    unsigned int n_out_of_date = 0;
};
} // namespace horizon
