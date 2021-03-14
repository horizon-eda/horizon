#pragma once
#include "nlohmann/json_fwd.hpp"
#include "util/uuid.hpp"
#include "common/lut.hpp"

namespace horizon {
enum class ObjectType;
using json = nlohmann::json;
class PoolIndex {
public:
    PoolIndex(const UUID &uu, const json &j);
    UUID uuid;
    std::string name;
    std::vector<UUID> included;
    enum class Level { CORE, EXTRA, COMMUNITY };
    Level level;
    std::string gh_user;
    std::string gh_repo;
    std::map<ObjectType, unsigned int> type_stats;

    static const LutEnumStr<Level> level_lut;
};
} // namespace horizon
