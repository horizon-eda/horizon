#pragma once
#include <nlohmann/json_fwd.hpp>
#include "parameter/set.hpp"
#include "pool/padstack.hpp"
#include "util/placement.hpp"
#include "util/uuid.hpp"
#include "util/uuid_ptr.hpp"

namespace horizon {
using json = nlohmann::json;

class Pad {
public:
    Pad(const UUID &uu, const json &, class IPool &pool);
    Pad(const UUID &uu, std::shared_ptr<const Padstack> ps);
    UUID uuid;
    std::shared_ptr<const Padstack> pool_padstack;
    Padstack padstack;
    Placement placement;
    std::string name;
    ParameterSet parameter_set;
    std::set<ParameterID> parameters_fixed;

    uuid_ptr<class Net> net = nullptr;
    bool is_nc = false;
    std::string secondary_text;

    UUID get_uuid() const;
    json serialize() const;
};
} // namespace horizon
