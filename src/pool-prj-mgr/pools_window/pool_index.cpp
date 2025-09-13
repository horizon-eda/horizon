#include "pool_index.hpp"
#include <nlohmann/json.hpp>
#include "common/common.hpp"

namespace horizon {
const LutEnumStr<PoolIndex::Level> PoolIndex::level_lut = {
        {"core", PoolIndex::Level::CORE},
        {"extra", PoolIndex::Level::EXTRA},
        {"community", PoolIndex::Level::COMMUNITY},
};

PoolIndex::PoolIndex(const UUID &uu, const json &j) : uuid(uu), name(j.at("name"))
{
    for (const auto &it : j.at("included")) {
        included.emplace_back(it.get<std::string>());
    }
    level = level_lut.lookup(j.at("level").get<std::string>());
    const auto source = j.at("source").at("type").get<std::string>();
    if (source != "github") {
        throw std::runtime_error("unsupported source " + source);
    }
    gh_user = j.at("source").at("user").get<std::string>();
    gh_repo = j.at("source").at("repo").get<std::string>();
    for (const auto &[k, v] : j.at("type_stats").items()) {
        type_stats.emplace(object_type_lut.lookup(k), v.get<unsigned int>());
    }
}

} // namespace horizon
