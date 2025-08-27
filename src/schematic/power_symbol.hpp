#pragma once
#include "util/uuid.hpp"
#include <nlohmann/json_fwd.hpp>
#include "common/common.hpp"
#include "util/uuid_ptr.hpp"

namespace horizon {
using json = nlohmann::json;


/**
 * GND symbols and the like. A PowerSymbol is associated with a power Net and
 * will make
 * its junction connected to this Net.
 */
class PowerSymbol {
public:
    PowerSymbol(const UUID &uu, const json &j, class Sheet *sheet = nullptr, class Block *block = nullptr);
    PowerSymbol(const UUID &uu);

    UUID uuid;
    uuid_ptr<class SchematicJunction> junction;
    uuid_ptr<class Net> net;
    bool mirror = false;
    Orientation orientation = Orientation::DOWN;
    void mirrorx();

    UUID get_uuid() const;
    void update_refs(Sheet &sheet, Block &block);

    json serialize() const;
};
} // namespace horizon
