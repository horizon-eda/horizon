#pragma once
#include <nlohmann/json_fwd.hpp>
#include "util/uuid.hpp"
#include "util/uuid_ptr.hpp"
#include "util/uuid_vec.hpp"
#include "pool/unit.hpp"

namespace horizon {
using json = nlohmann::json;

class NetTie {
public:
    NetTie(const UUID &uu, const json &, class Block &block);
    NetTie(const UUID &uu, const json &);
    NetTie(const UUID &uu);

    UUID uuid;
    UUID get_uuid() const;

    uuid_ptr<class Net> net_primary;
    uuid_ptr<class Net> net_secondary;

    void update_refs(Block &block);

    json serialize() const;
};
} // namespace horizon
