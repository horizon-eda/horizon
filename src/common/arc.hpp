#pragma once
#include "util/uuid.hpp"
#include <nlohmann/json_fwd.hpp>
#include "common.hpp"
#include "junction.hpp"
#include "util/uuid_ptr.hpp"

namespace horizon {
using json = nlohmann::json;

/**
 * Graphical arc. Same as a Line, an Arc is decorative only.
 * Since the Arc is over-dimensioned by three coordinates,
 * the renderer may choose not to render it when its coordinates
 * don't form a correct arc.
 */
class Arc {
public:
    Arc(const UUID &uu, const json &j, class ObjectProvider &obj);
    Arc(UUID uu);
    void reverse();
    std::pair<Coordi, Coordi> get_bbox() const;

    UUID uuid;
    uuid_ptr<Junction> to;
    uuid_ptr<Junction> from;
    uuid_ptr<Junction> center;
    uint64_t width = 0;
    int layer = 0;
    json serialize() const;
};
} // namespace horizon
