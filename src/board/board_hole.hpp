#pragma once
#include "nlohmann/json_fwd.hpp"
#include "parameter/set.hpp"
#include "pool/padstack.hpp"
#include "util/placement.hpp"
#include "util/uuid.hpp"
#include "util/uuid_ptr.hpp"

namespace horizon {
using json = nlohmann::json;

class BoardHole {
public:
    BoardHole(const UUID &uu, const json &, class Block *block, class IPool &pool);
    BoardHole(const UUID &uu, std::shared_ptr<const Padstack> ps);
    UUID uuid;
    std::shared_ptr<const Padstack> pool_padstack;
    Padstack padstack;
    Placement placement;
    ParameterSet parameter_set;

    uuid_ptr<class Net> net;

    UUID get_uuid() const;
    json serialize() const;
};
} // namespace horizon
