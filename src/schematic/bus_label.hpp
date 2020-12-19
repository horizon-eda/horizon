#pragma once
#include "util/uuid.hpp"
#include "nlohmann/json_fwd.hpp"
#include "common/common.hpp"
#include "util/uuid_ptr.hpp"
#include "nlohmann/json_fwd.hpp"
#include <set>

namespace horizon {
using json = nlohmann::json;

/**
 * Makes a Bus available on the schematic. Contrary to NetLabel, a BusLabel
 * forces its Bus on the attached Junction.
 */
class BusLabel {
public:
    BusLabel(const UUID &uu, const json &j, class Sheet &sheet, class Block &block);
    BusLabel(const UUID &uu, const json &j);
    BusLabel(const UUID &uu);

    UUID uuid;

    enum class Style { PLAIN, FLAG };
    Style style = Style::FLAG;

    uuid_ptr<class SchematicJunction> junction;
    Orientation orientation = Orientation::RIGHT;
    uint64_t size = 1.5_mm;
    std::set<unsigned int> on_sheets;
    bool offsheet_refs = true;
    uuid_ptr<class Bus> bus;

    json serialize() const;
};
} // namespace horizon
