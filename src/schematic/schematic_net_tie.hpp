#pragma once
#include "util/uuid.hpp"
#include "nlohmann/json_fwd.hpp"
#include "util/uuid_ptr.hpp"

namespace horizon {
using json = nlohmann::json;


class SchematicNetTie {
public:
    SchematicNetTie(const UUID &uu, const json &j, class Sheet &sheet, class Block &block);
    SchematicNetTie(const UUID &uu);
    UUID uuid;
    UUID get_uuid() const;

    void update_refs(class Sheet &sheet);

    uuid_ptr<class NetTie> net_tie;
    uuid_ptr<class SchematicJunction> from;
    uuid_ptr<class SchematicJunction> to;

    json serialize() const;
};
} // namespace horizon
