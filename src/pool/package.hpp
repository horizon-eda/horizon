#pragma once
#include "common/arc.hpp"
#include "common/common.hpp"
#include "common/hole.hpp"
#include "common/junction.hpp"
#include "common/layer_provider.hpp"
#include "common/line.hpp"
#include "common/object_provider.hpp"
#include "common/polygon.hpp"
#include "common/text.hpp"
#include "common/keepout.hpp"
#include "common/dimension.hpp"
#include "nlohmann/json_fwd.hpp"
#include "package/package_rules.hpp"
#include "package/pad.hpp"
#include "util/uuid.hpp"
#include "util/uuid_provider.hpp"
#include "util/warning.hpp"
#include "parameter/program_polygon.hpp"
#include "common/picture.hpp"
#include <fstream>
#include <map>
#include <set>
#include <vector>

namespace horizon {
using json = nlohmann::json;

class Package : public ObjectProvider, public LayerProvider, public UUIDProvider {
public:
    class MyParameterProgram : public ParameterProgramPolygon {
        friend Package;

    protected:
        std::map<UUID, Polygon> &get_polygons() override;

    private:
        ParameterProgram::CommandHandler get_command(const std::string &cmd) override;
        class Package *pkg = nullptr;

    public:
        MyParameterProgram(class Package *p, const std::string &code);
    };

    class Model {
    public:
        Model(const UUID &uu, const std::string &filename);
        Model(const UUID &uu, const json &j);

        UUID uuid;
        std::string filename;

        int64_t x = 0;
        int64_t y = 0;
        int64_t z = 0;

        int roll = 0;
        int pitch = 0;
        int yaw = 0;

        json serialize() const;
    };

    Package(const UUID &uu, const json &j, class Pool &pool);
    Package(const UUID &uu);
    static Package new_from_file(const std::string &filename, class Pool &pool);

    json serialize() const;
    Junction *get_junction(const UUID &uu) override;
    Polygon *get_polygon(const UUID &uu) override;
    std::pair<Coordi, Coordi> get_bbox() const;
    const std::map<int, Layer> &get_layers() const override;
    std::pair<bool, std::string> apply_parameter_set(const ParameterSet &ps);
    void expand();
    void update_warnings();
    int get_max_pad_name() const;

    UUID get_uuid() const override;

    Package(const Package &pkg);
    void operator=(Package const &pkg);

    UUID uuid;
    std::string name;
    std::string manufacturer;
    std::set<std::string> tags;

    std::map<UUID, Junction> junctions;
    std::map<UUID, Line> lines;
    std::map<UUID, Arc> arcs;
    std::map<UUID, Text> texts;
    std::map<UUID, Pad> pads;
    std::map<UUID, Polygon> polygons;
    std::map<UUID, Keepout> keepouts;
    std::map<UUID, Dimension> dimensions;
    std::map<UUID, Picture> pictures;

    ParameterSet parameter_set;
    MyParameterProgram parameter_program;
    PackageRules rules;

    std::map<UUID, Model> models;
    UUID default_model;
    const Model *get_model(const UUID &uu = UUID()) const;

    const class Package *alternate_for = nullptr;

    std::vector<Warning> warnings;

    void update_refs(class Pool &pool);

    void save_pictures(const std::string &dir) const;
    void load_pictures(const std::string &dir);

private:
    void update_refs();
};
} // namespace horizon
