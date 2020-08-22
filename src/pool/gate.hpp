#pragma once
#include "nlohmann/json_fwd.hpp"
#include "unit.hpp"
#include "util/uuid.hpp"
#include "util/uuid_ptr.hpp"
#include <fstream>
#include <map>
#include <vector>

namespace horizon {
using json = nlohmann::json;

class Gate {
public:
    Gate(const UUID &uu, const json &, class IPool &pool);
    Gate(const UUID &uu);
    UUID get_uuid() const;
    UUID uuid;
    std::string name;
    std::string suffix;
    unsigned int swap_group = 0;
    uuid_ptr<const Unit> unit;

    json serialize() const;
};
} // namespace horizon
