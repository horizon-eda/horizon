#pragma once
#include "nlohmann/json_fwd.hpp"
#include "parameter/set.hpp"
#include "pool/padstack.hpp"
#include "util/placement.hpp"
#include "util/uuid.hpp"
#include "util/uuid_provider.hpp"
#include "util/uuid_ptr.hpp"
#include <fstream>
#include <map>
#include <vector>

namespace horizon {
using json = nlohmann::json;

class BoardHole : public UUIDProvider {
public:
    BoardHole(const UUID &uu, const json &, class Block &block, class Pool &pool);
    BoardHole(const UUID &uu, const Padstack *ps);
    UUID uuid;
    const Padstack *pool_padstack;
    Padstack padstack;
    Placement placement;
    ParameterSet parameter_set;

    uuid_ptr<Net> net = nullptr;

    virtual UUID get_uuid() const;
    json serialize() const;
};
} // namespace horizon
