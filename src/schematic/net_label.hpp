#pragma once
#include "util/uuid.hpp"
#include <nlohmann/json_fwd.hpp>
#include "common/common.hpp"
#include "util/uuid_ptr.hpp"
#include <set>

namespace horizon {
using json = nlohmann::json;

/**
 * Displays the junction's Net name it is attached to. A net
 * label doesn't 'force' its Net on the attached junction, it merely
 * tells its Net name.
 */
class NetLabel {
public:
    NetLabel(const UUID &uu, const json &j, class Sheet *sheet = nullptr);
    NetLabel(const UUID &uu);

    UUID uuid;

    enum class Style { PLAIN, FLAG };
    Style style = Style::FLAG;

    uuid_ptr<class SchematicJunction> junction;
    Orientation orientation = Orientation::RIGHT;
    uint64_t size = 1.5_mm;
    std::set<unsigned int> on_sheets;
    bool offsheet_refs = true;
    bool show_port = false;
    UUID last_net;

    json serialize() const;
};
} // namespace horizon
