#pragma once
#include <set>
#include "common/common.hpp"
#include "common/lut.hpp"
#include "nlohmann/json_fwd.hpp"
#include "util/uuid.hpp"

namespace horizon {
using json = nlohmann::json;

class STEPExportSettings {
public:
    STEPExportSettings(const json &);
    STEPExportSettings()
    {
    }
    json serialize() const;

    std::string filename;
    std::string prefix;
    bool include_3d_models = true;
    uint64_t min_diameter;
    std::set<UUID> pkgs_excluded;
};
} // namespace horizon
