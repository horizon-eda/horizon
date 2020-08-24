#pragma once
#include "common/common.hpp"
#include "common/layer_provider.hpp"
#include "common/polygon.hpp"
#include "common/line.hpp"
#include "common/arc.hpp"
#include "common/text.hpp"
#include "nlohmann/json_fwd.hpp"
#include "util/uuid.hpp"
#include "common/object_provider.hpp"

namespace horizon {
using json = nlohmann::json;

class Decal : public ObjectProvider, public LayerProvider {
public:
    Decal(const UUID &uu, const json &j);
    Decal(const UUID &uu);
    static Decal new_from_file(const std::string &filename);

    json serialize() const;

    Decal(const Decal &other);
    void operator=(Decal const &other);

    UUID uuid;
    std::string name;
    std::map<UUID, Junction> junctions;
    std::map<UUID, Polygon> polygons;
    std::map<UUID, Line> lines;
    std::map<UUID, Arc> arcs;
    std::map<UUID, Text> texts;

    UUID get_uuid() const;
    std::pair<Coordi, Coordi> get_bbox() const;
    const std::map<int, Layer> &get_layers() const override;
    Junction *get_junction(const UUID &uu) override;


private:
    void update_refs();
};
} // namespace horizon
