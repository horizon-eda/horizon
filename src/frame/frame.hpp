#pragma once
#include "common/arc.hpp"
#include "common/common.hpp"
#include "common/junction.hpp"
#include "common/layer_provider.hpp"
#include "common/line.hpp"
#include "common/object_provider.hpp"
#include "common/polygon.hpp"
#include "common/text.hpp"
#include "nlohmann/json_fwd.hpp"
#include "util/uuid.hpp"
#include "util/uuid_provider.hpp"
#include <fstream>
#include <map>
#include <set>
#include <vector>

namespace horizon {
using json = nlohmann::json;

class Frame : public ObjectProvider, public LayerProvider, public UUIDProvider {
public:
    Frame(const UUID &uu, const json &j);
    Frame(const UUID &uu);
    static Frame new_from_file(const std::string &filename);
    std::pair<Coordi, Coordi> get_bbox(bool all = false) const;
    Junction *get_junction(const UUID &uu) override;
    UUID get_uuid() const override;

    json serialize() const;

    void expand();
    Frame(const Frame &sym);
    void operator=(Frame const &sym);

    UUID uuid;
    std::string name;
    std::map<UUID, Junction> junctions;
    std::map<UUID, Line> lines;
    std::map<UUID, Arc> arcs;
    std::map<UUID, Text> texts;
    std::map<UUID, Polygon> polygons;

    int64_t width = 100_mm;
    int64_t height = 100_mm;

private:
    void update_refs();
};
} // namespace horizon
