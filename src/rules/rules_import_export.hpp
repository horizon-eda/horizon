#pragma once
#include "nlohmann/json_fwd.hpp"
#include "util/uuid.hpp"

namespace horizon {
using json = nlohmann::json;
class RulesImportInfo {
public:
    RulesImportInfo(const json &j);
    virtual const json &get_rules() const = 0;
    std::string name;
    std::string notes;
    virtual ~RulesImportInfo()
    {
    }
};

class RulesExportInfo {
public:
    RulesExportInfo(const json &j);
    RulesExportInfo();
    std::string name;
    std::string notes;
    UUID uuid;
    void serialize(json &j) const;
};

} // namespace horizon
