#include "padstack.hpp"
#include "board/board_layers.hpp"
#include "common/lut.hpp"
#include "util/util.hpp"
#include "nlohmann/json.hpp"
#include "util/bbox_accumulator.hpp"

namespace horizon {

const LutEnumStr<Padstack::Type> Padstack::type_lut = {
        {"top", Padstack::Type::TOP}, {"bottom", Padstack::Type::BOTTOM}, {"through", Padstack::Type::THROUGH},
        {"via", Padstack::Type::VIA}, {"hole", Padstack::Type::HOLE},     {"mechanical", Padstack::Type::MECHANICAL}};

Padstack::MyParameterProgram::MyParameterProgram(Padstack *p, const std::string &c) : ParameterProgramPolygon(c), ps(p)
{
}

std::optional<std::string> Padstack::MyParameterProgram::set_shape(const TokenCommand &cmd)
{
    if (cmd.arguments.size() < 2 || cmd.arguments.at(0)->type != Token::Type::STR
        || cmd.arguments.at(1)->type != Token::Type::STR)
        return "not enough arguments";

    const auto &pclass = dynamic_cast<TokenString &>(*cmd.arguments.at(0).get()).string;
    const auto &form = dynamic_cast<TokenString &>(*cmd.arguments.at(1).get()).string;

    if (form == "rectangle") {
        int64_t width, height;
        if (stack_pop(height) || stack_pop(width))
            return "empty stack";
        for (auto &it : ps->shapes) {
            if (it.second.parameter_class == pclass) {
                it.second.form = Shape::Form::RECTANGLE;
                it.second.params = {width, height};
            }
        }
    }
    else if (form == "circle") {
        int64_t diameter;
        if (stack_pop(diameter))
            return "empty stack";
        for (auto &it : ps->shapes) {
            if (it.second.parameter_class == pclass) {
                it.second.form = Shape::Form::CIRCLE;
                it.second.params = {diameter};
            }
        }
    }
    else if (form == "obround") {
        int64_t width, height;
        if (stack_pop(height) || stack_pop(width))
            return "empty stack";
        for (auto &it : ps->shapes) {
            if (it.second.parameter_class == pclass) {
                it.second.form = Shape::Form::OBROUND;
                it.second.params = {width, height};
            }
        }
    }
    else if (form == "position") {
        int64_t x, y;
        if (stack_pop(y) || stack_pop(x))
            return "empty stack";
        for (auto &it : ps->shapes) {
            if (it.second.parameter_class == pclass) {
                it.second.placement.shift = {x, y};
            }
        }
    }

    else {
        return "unknown form " + form;
    }

    return {};
}

std::optional<std::string> Padstack::MyParameterProgram::set_hole(const TokenCommand &cmd)
{
    if (cmd.arguments.size() < 2 || cmd.arguments.at(0)->type != Token::Type::STR
        || cmd.arguments.at(1)->type != Token::Type::STR)
        return "not enough arguments";

    const auto &pclass = dynamic_cast<TokenString &>(*cmd.arguments.at(0).get()).string;
    const auto &shape = dynamic_cast<TokenString &>(*cmd.arguments.at(1).get()).string;

    if (shape == "round") {
        int64_t diameter;
        if (stack_pop(diameter))
            return "empty stack";
        for (auto &it : ps->holes) {
            if (it.second.parameter_class == pclass) {
                it.second.shape = Hole::Shape::ROUND;
                it.second.diameter = diameter;
            }
        }
    }
    else if (shape == "slot") {
        int64_t diameter, length;
        if (stack_pop(length) || stack_pop(diameter))
            return "empty stack";
        for (auto &it : ps->holes) {
            if (it.second.parameter_class == pclass) {
                it.second.shape = Hole::Shape::SLOT;
                it.second.diameter = diameter;
                it.second.length = length;
            }
        }
    }
    else if (shape == "position") {
        int64_t x, y;
        if (stack_pop(y) || stack_pop(x))
            return "empty stack";
        for (auto &it : ps->holes) {
            if (it.second.parameter_class == pclass) {
                it.second.placement.shift = {x, y};
            }
        }
    }

    else {
        return "unknown shape " + shape;
    }

    return {};
}

std::map<UUID, Polygon> &Padstack::MyParameterProgram::get_polygons()
{
    return ps->polygons;
}


ParameterProgram::CommandHandler Padstack::MyParameterProgram::get_command(const std::string &cmd)
{
    using namespace std::placeholders;
    if (auto r = ParameterProgramPolygon::get_command(cmd)) {
        return r;
    }
    else if (cmd == "set-shape") {
        return static_cast<CommandHandler>(&Padstack::MyParameterProgram::set_shape);
    }
    else if (cmd == "set-hole") {
        return static_cast<CommandHandler>(&Padstack::MyParameterProgram::set_hole);
    }
    return nullptr;
}

Padstack::Padstack(const Padstack &ps)
    : uuid(ps.uuid), name(ps.name), type(ps.type), polygons(ps.polygons), holes(ps.holes), shapes(ps.shapes),
      parameter_set(ps.parameter_set), parameters_required(ps.parameters_required),
      parameter_program(ps.parameter_program), version(ps.version)
{
    update_refs();
}

void Padstack::operator=(Padstack const &ps)
{
    uuid = ps.uuid;
    name = ps.name;
    type = ps.type;
    polygons = ps.polygons;
    holes = ps.holes;
    shapes = ps.shapes;
    parameter_set = ps.parameter_set;
    parameters_required = ps.parameters_required;
    parameter_program = ps.parameter_program;
    version = ps.version;
    update_refs();
}

void Padstack::update_refs()
{
    parameter_program.ps = this;
}

static const unsigned int app_version = 0;

unsigned int Padstack::get_app_version()
{
    return app_version;
}

Padstack::Padstack(const UUID &uu, const json &j)
    : uuid(uu), name(j.at("name").get<std::string>()), well_known_name(j.value("well_known_name", "")),
      parameter_program(this, j.value("parameter_program", "")), version(app_version, j)
{
    check_object_type(j, ObjectType::PADSTACK);
    version.check(ObjectType::PADSTACK, name, uuid);
    {
        const json &o = j["polygons"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            polygons.emplace(std::make_pair(u, Polygon(u, it.value())));
        }
    }
    map_erase_if(polygons, [](const auto &a) { return a.second.vertices.size() == 0; });
    {
        const json &o = j["holes"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            holes.emplace(std::make_pair(u, Hole(u, it.value())));
        }
    }
    if (j.count("shapes")) {
        const json &o = j["shapes"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            shapes.emplace(std::make_pair(u, Shape(u, it.value())));
        }
    }
    if (j.count("padstack_type")) {
        type = type_lut.lookup(j.at("padstack_type"));
    }
    if (j.count("parameter_set")) {
        parameter_set = parameter_set_from_json(j.at("parameter_set"));
    }
    if (j.count("parameters_required")) {
        const json &o = j["parameters_required"];
        for (const auto &value : o) {
            parameters_required.insert(parameter_id_from_string(value.get<std::string>()));
        }
    }
}

Padstack Padstack::new_from_file(const std::string &filename)
{
    auto j = load_json_from_file(filename);
    return Padstack(UUID(j.at("uuid").get<std::string>()), j);
}

Padstack::Padstack(const UUID &uu) : uuid(uu), parameter_program(this, ""), version(app_version)
{
}

json Padstack::serialize() const
{
    json j;
    version.serialize(j);
    j["uuid"] = (std::string)uuid;
    j["type"] = "padstack";
    j["name"] = name;
    j["well_known_name"] = well_known_name;
    j["padstack_type"] = type_lut.lookup_reverse(type);
    j["parameter_program"] = parameter_program.get_code();
    j["parameter_set"] = parameter_set_serialize(parameter_set);
    j["polygons"] = json::object();
    for (const auto &it : polygons) {
        j["polygons"][(std::string)it.first] = it.second.serialize();
    }
    j["holes"] = json::object();
    for (const auto &it : holes) {
        j["holes"][(std::string)it.first] = it.second.serialize();
    }
    j["shapes"] = json::object();
    for (const auto &it : shapes) {
        j["shapes"][(std::string)it.first] = it.second.serialize();
    }
    j["parameters_required"] = json::array();
    for (const auto &it : parameters_required) {
        j["parameters_required"].push_back(parameter_id_to_string(it));
    }
    return j;
}

UUID Padstack::get_uuid() const
{
    return uuid;
}

std::pair<Coordi, Coordi> Padstack::get_bbox(bool copper_only) const
{
    BBoxAccumulator<Coordi::type> acc;
    for (const auto &it : polygons) {
        if (!copper_only || BoardLayers::is_copper(it.second.layer)) {
            acc.accumulate(it.second.get_bbox());
        }
    }
    for (const auto &it : shapes) {
        if (!copper_only || BoardLayers::is_copper(it.second.layer)) {
            const auto bb = it.second.placement.transform_bb(it.second.get_bbox());
            acc.accumulate(bb);
        }
    }
    if (!copper_only) {
        for (const auto &it : holes) {
            const auto bb = it.second.placement.transform_bb(it.second.get_bbox());
            acc.accumulate(bb);
        }
    }
    return acc.get_or_0();
}

const std::map<int, Layer> &Padstack::get_layers() const
{
    static const std::map<int, Layer> layers = {{30, {30, "Top Paste"}},
                                                {10, {10, "Top Mask"}},
                                                {0, {0, "Top Copper", false, true}},
                                                {-1, {-1, "Inner", false, true}},
                                                {-100, {-100, "Bottom Copper", true, true}},
                                                {-110, {-110, "Bottom Mask", true}},
                                                {-130, {-130, "Bottom Paste"}}};
    return layers;
}

static void copy_param(ParameterSet &dest, const ParameterSet &src, ParameterID id)
{
    if (src.count(id))
        dest[id] = src.at(id);
}

static void copy_param(ParameterSet &dest, const ParameterSet &src, const std::set<ParameterID> &ids)
{
    for (const auto id : ids) {
        copy_param(dest, src, id);
    }
}

std::optional<std::string> Padstack::apply_parameter_set(const ParameterSet &ps)
{
    auto ps_this = parameter_set;
    copy_param(ps_this, ps,
               {ParameterID::PAD_HEIGHT, ParameterID::PAD_WIDTH, ParameterID::PAD_DIAMETER,
                ParameterID::SOLDER_MASK_EXPANSION, ParameterID::PASTE_MASK_CONTRACTION, ParameterID::HOLE_DIAMETER,
                ParameterID::HOLE_LENGTH, ParameterID::VIA_DIAMETER, ParameterID::HOLE_SOLDER_MASK_EXPANSION,
                ParameterID::VIA_SOLDER_MASK_EXPANSION, ParameterID::HOLE_ANNULAR_RING, ParameterID::CORNER_RADIUS});
    return parameter_program.run(ps_this);
}

static UUID get_layer_uuid(const UUID &ns, const UUID &uu, uint32_t index)
{
    std::array<unsigned char, UUID::size + sizeof(index)> buf;
    memcpy(buf.data(), uu.get_bytes(), UUID::size);
    memcpy(buf.data() + UUID::size, &index, sizeof(index));
    return UUID::UUID5(ns, buf.data(), buf.size());
}

void Padstack::expand_inner(unsigned int n_inner, const LayerRange &span)
{
    for (auto &[uu, it] : holes) {
        it.span = span;
    }

    static const std::vector<std::pair<int, int>> layers = {
            {BoardLayers::TOP_COPPER, BoardLayers::TOP_MASK},
            {BoardLayers::BOTTOM_COPPER, BoardLayers::BOTTOM_MASK},
    };
    for (const auto &it : layers) {
        const auto layer_cu = it.first;
        const auto layer_mask = it.second;
        if (!span.overlaps(layer_cu)) {
            map_erase_if(shapes, [layer_cu, layer_mask](const auto &el) {
                return any_of(el.second.layer, {layer_cu, layer_mask});
            });
            map_erase_if(polygons, [layer_cu, layer_mask](const auto &el) {
                return any_of(el.second.layer, {layer_cu, layer_mask});
            });
        }
    }
    std::map<UUID, Polygon> new_polygons;
    std::map<UUID, Shape> new_shapes;

    for (int i = 0; i < (((int)n_inner) - 1); i++) {
        for (const auto &it : polygons) {
            if (it.second.layer == -1) {

                const int layer = -2 - i;
                if (span.overlaps(layer)) {
                    const auto uu = get_layer_uuid(UUID{"7ba04a7a-7644-4bdf-ba8d-6bc006fb6ae6"}, it.first, i);
                    auto &np = new_polygons.emplace(uu, it.second).first->second;
                    np.uuid = uu;
                    np.layer = layer;
                }
            }
        }

        for (const auto &it : shapes) {
            if (it.second.layer == -1) {
                const int layer = -2 - i;
                if (span.overlaps(layer)) {
                    const auto uu = get_layer_uuid(UUID{"81dca5e4-5215-4072-892e-9883265e90b2"}, it.first, i);
                    auto &np = new_shapes.emplace(uu, it.second).first->second;
                    np.uuid = uu;
                    np.layer = layer;
                }
            }
        }
    }
    polygons.insert(new_polygons.begin(), new_polygons.end());
    shapes.insert(new_shapes.begin(), new_shapes.end());

    if (n_inner == 0 || !span.overlaps((-1))) {
        map_erase_if(shapes, [](const auto &it) { return it.second.layer == -1; });
        map_erase_if(polygons, [](const auto &it) { return it.second.layer == -1; });
    }
}
} // namespace horizon
