#pragma once
#include "gate.hpp"
#include "nlohmann/json_fwd.hpp"
#include "unit.hpp"
#include "util/uuid.hpp"
#include <fstream>
#include <map>
#include <vector>
#include "util/file_version.hpp"

namespace horizon {
using json = nlohmann::json;

class Entity {
private:
    Entity(const UUID &uu, const json &, class IPool &pool);

public:
    Entity(const UUID &uu);

    static Entity new_from_file(const std::string &filename, IPool &pool);
    static unsigned int get_app_version();
    UUID uuid;
    std::string name;
    std::string manufacturer;
    std::string prefix;
    std::set<std::string> tags;
    std::map<UUID, Gate> gates;

    FileVersion version;
    json serialize() const;
    void update_refs(IPool &pool);
    UUID get_uuid() const;
};
} // namespace horizon
