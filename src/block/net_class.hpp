#pragma once
#include "nlohmann/json_fwd.hpp"
#include "util/uuid.hpp"

namespace horizon {
using json = nlohmann::json;

class NetClass {
public:
    NetClass(const UUID &uu, const json &j);
    NetClass(const UUID &uu);
    UUID uuid;
    std::string name;
    bool is_default = false;
    UUID get_uuid() const;

    json serialize() const;
};
} // namespace horizon
