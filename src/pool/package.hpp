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
#include "util/warning.hpp"
#include "parameter/program_polygon.hpp"
#include "common/picture.hpp"
#include <map>
#include <set>
#include <vector>
#include "util/file_version.hpp"
#include "common/grid_settings.hpp"

namespace horizon {
using json = nlohmann::json;

class Package : public ObjectProvider, public LayerProvider {
public:
    class MyParameterProgram : public ParameterProgramPolygon {
        friend Package;

    protected:
        std::map<UUID, Polygon> &get_polygons() override;

    private:
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

        void set_shift(unsigned int ax, int64_t value);
        int64_t get_shift(unsigned int ax) const;

        int roll = 0;
        int pitch = 0;
        int yaw = 0;

        void set_rotation(unsigned int ax, int angle);
        int get_rotation(unsigned int ax) const;

        static const UUID legacy_model_uuid;

        json serialize() const;
    };

    Package(const UUID &uu, const json &j, class IPool &pool);
    Package(const UUID &uu);
    static Package new_from_file(const std::string &filename, class IPool &pool);
    static unsigned int get_app_version();

    json serialize() const;
    Junction *get_junction(const UUID &uu) override;
    Polygon *get_polygon(const UUID &uu) override;
    std::pair<Coordi, Coordi> get_bbox() const;
    const std::map<int, Layer> &get_layers() const override;
    std::optional<std::string> apply_parameter_set(const ParameterSet &ps);
    void expand();
    void update_warnings();
    int get_max_pad_name() const;

    UUID get_uuid() const;

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
    GridSettings grid_settings;

    std::map<UUID, Model> models;
    UUID default_model;
    const Model *get_model(const UUID &uu = UUID()) const;

    const class Package *alternate_for = nullptr;

    FileVersion version;

    std::vector<Warning> warnings;

    void update_refs(class IPool &pool);

    void save_pictures(const std::string &dir) const;
    void load_pictures(const std::string &dir);

    std::vector<Pad *> get_pads_sorted();
    std::vector<const Pad *> get_pads_sorted() const;

private:
    void update_refs();
};
} // namespace horizon
