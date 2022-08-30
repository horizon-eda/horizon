#pragma once
#include "util/uuid.hpp"
#include "util/placement.hpp"
#include "nlohmann/json_fwd.hpp"
#include <set>

namespace horizon {
using json = nlohmann::json;
class PastedPackage {
public:
    PastedPackage(const UUID &uu, const json &j);
    UUID uuid;
    Placement placement;
    bool flip;
    bool omit_silkscreen;
    bool smashed;
    UUID group;
    UUID tag;
    UUID package;
    UUID alternate_package;
    std::map<UUID, UUID> connections;
    std::set<UUID> texts;
};

} // namespace horizon
