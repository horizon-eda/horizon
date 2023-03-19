#pragma once
#include "common/common.hpp"
#include "common/hole.hpp"
#include "common/layer_provider.hpp"
#include "common/lut.hpp"
#include "common/polygon.hpp"
#include "common/shape.hpp"
#include "nlohmann/json_fwd.hpp"
#include "parameter/program.hpp"
#include "parameter/program_polygon.hpp"
#include "util/uuid.hpp"
#include <map>
#include <set>
#include "util/file_version.hpp"

namespace horizon {
using json = nlohmann::json;

class Padstack : public LayerProvider {
public:
    class MyParameterProgram : public ParameterProgramPolygon {
        friend Padstack;

    protected:
        std::map<UUID, Polygon> &get_polygons() override;

    private:
        ParameterProgram::CommandHandler get_command(const std::string &cmd) override;
        class Padstack *ps = nullptr;

        std::optional<std::string> set_shape(const TokenCommand &cmd);
        std::optional<std::string> set_hole(const TokenCommand &cmd);

    public:
        MyParameterProgram(class Padstack *p, const std::string &code);
    };

    enum class Type { TOP, BOTTOM, THROUGH, VIA, HOLE, MECHANICAL };
    static const LutEnumStr<Padstack::Type> type_lut;

    Padstack(const UUID &uu, const json &j);
    Padstack(const UUID &uu);
    static Padstack new_from_file(const std::string &filename);
    static unsigned int get_app_version();

    json serialize() const;

    Padstack(const Padstack &ps);
    void operator=(Padstack const &ps);

    UUID uuid;
    std::string name;
    std::string well_known_name;
    Type type = Type::TOP;
    std::map<UUID, Polygon> polygons;
    std::map<UUID, Hole> holes;
    std::map<UUID, Shape> shapes;

    ParameterSet parameter_set;
    std::set<ParameterID> parameters_required;
    MyParameterProgram parameter_program;

    FileVersion version;

    std::optional<std::string> apply_parameter_set(const ParameterSet &ps);

    UUID get_uuid() const;
    std::pair<Coordi, Coordi> get_bbox(bool copper_only = false) const;
    void expand_inner(unsigned int n_inner, const class LayerRange &span);
    const std::map<int, Layer> &get_layers() const override;

private:
    void update_refs();
};
} // namespace horizon
