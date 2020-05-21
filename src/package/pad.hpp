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

class Pad : public UUIDProvider {
public:
    Pad(const UUID &uu, const json &, class Pool &pool);
    Pad(const UUID &uu, const Padstack *ps);
    UUID uuid;
    uuid_ptr<const Padstack> pool_padstack;
    Padstack padstack;
    Placement placement;
    std::string name;
    ParameterSet parameter_set;

    uuid_ptr<Net> net = nullptr;
    bool is_nc = false;
    std::string secondary_text;

    virtual UUID get_uuid() const;
    json serialize() const;
};
} // namespace horizon
