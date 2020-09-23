#include "ipool.hpp"
#include "package.hpp"
#include "util/util.hpp"
#include "nlohmann/json.hpp"
#include "board/board_layers.hpp"
#include "util/picture_load.hpp"

namespace horizon {

Package::MyParameterProgram::MyParameterProgram(Package *p, const std::string &c) : ParameterProgramPolygon(c), pkg(p)
{
}

std::map<UUID, Polygon> &Package::MyParameterProgram::get_polygons()
{
    return pkg->polygons;
}

ParameterProgram::CommandHandler Package::MyParameterProgram::get_command(const std::string &cmd)
{
    using namespace std::placeholders;
    if (auto r = ParameterProgram::get_command(cmd)) {
        return r;
    }
    else if (cmd == "set-polygon") {
        return std::bind(std::mem_fn(&Package::MyParameterProgram::set_polygon), this, _1, _2);
    }
    else if (cmd == "set-polygon-vertices") {
        return std::bind(std::mem_fn(&Package::MyParameterProgram::set_polygon_vertices), this, _1, _2);
    }
    else if (cmd == "expand-polygon") {
        return std::bind(std::mem_fn(&Package::MyParameterProgram::expand_polygon), this, _1, _2);
    }
    return nullptr;
}

Package::Model::Model(const UUID &uu, const std::string &fn) : uuid(uu), filename(fn)
{
}
Package::Model::Model(const UUID &uu, const json &j)
    : uuid(uu), filename(j.at("filename").get<std::string>()), x(j.at("x")), y(j.at("y")), z(j.at("z")),
      roll(j.at("roll")), pitch(j.at("pitch")), yaw(j.at("yaw"))
{
}

json Package::Model::serialize() const
{
    json j;
    j["filename"] = filename;
    j["x"] = x;
    j["y"] = y;
    j["z"] = z;
    j["roll"] = roll;
    j["pitch"] = pitch;
    j["yaw"] = yaw;

    return j;
}

Package::Package(const UUID &uu, const json &j, IPool &pool)
    : uuid(uu), name(j.at("name").get<std::string>()), manufacturer(j.value("manufacturer", "")),
      parameter_program(this, j.value("parameter_program", ""))
{
    check_object_type(j, ObjectType::PACKAGE);
    {
        const json &o = j.at("junctions");
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            junctions.emplace(std::piecewise_construct, std::forward_as_tuple(u), std::forward_as_tuple(u, it.value()));
        }
    }
    {
        const json &o = j.at("lines");
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            lines.emplace(std::piecewise_construct, std::forward_as_tuple(u),
                          std::forward_as_tuple(u, it.value(), *this));
        }
    }
    {
        const json &o = j.at("arcs");
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            arcs.emplace(std::piecewise_construct, std::forward_as_tuple(u),
                         std::forward_as_tuple(u, it.value(), *this));
        }
    }
    {
        const json &o = j.at("texts");
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            texts.emplace(std::piecewise_construct, std::forward_as_tuple(u), std::forward_as_tuple(u, it.value()));
        }
    }
    {
        const json &o = j.at("pads");
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            pads.emplace(std::piecewise_construct, std::forward_as_tuple(u),
                         std::forward_as_tuple(u, it.value(), pool));
        }
    }

    if (j.count("polygons")) {
        const json &o = j["polygons"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            polygons.emplace(std::piecewise_construct, std::forward_as_tuple(u), std::forward_as_tuple(u, it.value()));
        }
    }
    if (j.count("keepouts")) {
        const json &o = j["keepouts"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            keepouts.emplace(std::piecewise_construct, std::forward_as_tuple(u),
                             std::forward_as_tuple(u, it.value(), *this));
        }
    }
    if (j.count("dimensions")) {
        const json &o = j["dimensions"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            dimensions.emplace(std::piecewise_construct, std::forward_as_tuple(u),
                               std::forward_as_tuple(u, it.value()));
        }
    }
    if (j.count("pictures")) {
        const json &o = j["pictures"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            pictures.emplace(std::piecewise_construct, std::forward_as_tuple(u), std::forward_as_tuple(u, it.value()));
        }
    }
    for (auto &it : keepouts) {
        it.second.polygon.update(polygons);
        it.second.polygon->usage = &it.second;
    }
    map_erase_if(polygons, [](const auto &a) { return a.second.vertices.size() == 0; });
    if (j.count("tags")) {
        tags = j.at("tags").get<std::set<std::string>>();
    }
    if (j.count("parameter_set")) {
        parameter_set = parameter_set_from_json(j.at("parameter_set"));
    }
    if (j.count("alternate_for")) {
        alternate_for = pool.get_package(UUID(j.at("alternate_for").get<std::string>()));
    }
    if (j.count("model_filename")) {
        std::string mfn = j.at("model_filename");
        if (mfn.size()) {
            auto m_uu = UUID("96c366ee-a963-41a0-9cc8-54c646979695");
            models.emplace(std::piecewise_construct, std::forward_as_tuple(m_uu),
                           std::forward_as_tuple(m_uu, j.at("model_filename").get<std::string>()));
            default_model = m_uu;
        }
    }
    if (j.count("models")) {
        const json &o = j["models"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            models.emplace(std::piecewise_construct, std::forward_as_tuple(u), std::forward_as_tuple(u, it.value()));
        }
        default_model = j.at("default_model").get<std::string>();
    }
    if (j.count("rules")) {
        rules.load_from_json(j.at("rules"));
    }
}

Package Package::new_from_file(const std::string &filename, IPool &pool)
{
    auto j = load_json_from_file(filename);
    return Package(UUID(j.at("uuid").get<std::string>()), j, pool);
}

Package::Package(const UUID &uu) : uuid(uu), parameter_program(this, "")
{
}

Junction *Package::get_junction(const UUID &uu)
{
    return &junctions.at(uu);
}

Polygon *Package::get_polygon(const UUID &uu)
{
    return &polygons.at(uu);
}

Package::Package(const Package &pkg)
    : uuid(pkg.uuid), name(pkg.name), manufacturer(pkg.manufacturer), tags(pkg.tags), junctions(pkg.junctions),
      lines(pkg.lines), arcs(pkg.arcs), texts(pkg.texts), pads(pkg.pads), polygons(pkg.polygons),
      keepouts(pkg.keepouts), dimensions(pkg.dimensions), pictures(pkg.pictures), parameter_set(pkg.parameter_set),
      parameter_program(pkg.parameter_program), models(pkg.models), default_model(pkg.default_model),
      alternate_for(pkg.alternate_for), warnings(pkg.warnings)
{
    update_refs();
}

void Package::operator=(Package const &pkg)
{
    uuid = pkg.uuid;
    name = pkg.name;
    manufacturer = pkg.manufacturer;
    tags = pkg.tags;
    junctions = pkg.junctions;
    lines = pkg.lines;
    arcs = pkg.arcs;
    texts = pkg.texts;
    pads = pkg.pads;
    polygons = pkg.polygons;
    keepouts = pkg.keepouts;
    dimensions = pkg.dimensions;
    pictures = pkg.pictures;
    parameter_set = pkg.parameter_set;
    parameter_program = pkg.parameter_program;
    models = pkg.models;
    default_model = pkg.default_model;
    alternate_for = pkg.alternate_for;
    warnings = pkg.warnings;
    update_refs();
}

void Package::update_refs(IPool &pool)
{
    for (auto &it : pads) {
        it.second.pool_padstack = pool.get_padstack(it.second.pool_padstack.uuid);
        it.second.padstack = *it.second.pool_padstack;
    }
    update_refs();
}

void Package::update_refs()
{
    for (auto &it : lines) {
        auto &line = it.second;
        line.to = &junctions.at(line.to.uuid);
        line.from = &junctions.at(line.from.uuid);
    }
    for (auto &it : arcs) {
        auto &arc = it.second;
        arc.to = &junctions.at(arc.to.uuid);
        arc.from = &junctions.at(arc.from.uuid);
        arc.center = &junctions.at(arc.center.uuid);
    }
    for (auto &it : keepouts) {
        it.second.polygon.update(polygons);
        it.second.polygon->usage = &it.second;
    }
    parameter_program.pkg = this;
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

std::pair<bool, std::string> Package::apply_parameter_set(const ParameterSet &ps)
{
    auto ps_this = parameter_set;
    copy_param(ps_this, ps, ParameterID::COURTYARD_EXPANSION);
    {
        auto r = parameter_program.run(ps_this);
        if (r.first) {
            return r;
        }
    }

    for (auto &it : pads) {
        auto ps_pad = it.second.parameter_set;
        copy_param(ps_pad, ps,
                   {ParameterID::SOLDER_MASK_EXPANSION, ParameterID::PASTE_MASK_CONTRACTION,
                    ParameterID::HOLE_SOLDER_MASK_EXPANSION});
        auto r = it.second.padstack.apply_parameter_set(ps_pad);
        if (r.first) {
            return {r.first, "Pad " + it.second.name + ": " + r.second};
        }
    }
    return {false, ""};
}

void Package::expand()
{
    map_erase_if(keepouts, [this](auto &it) { return polygons.count(it.second.polygon.uuid) == 0; });
    for (auto &it : junctions) {
        it.second.layer = 10000;
        it.second.has_via = false;
        it.second.needs_via = false;
        it.second.connection_count = 0;
    }

    for (const auto &it : lines) {
        it.second.from->connection_count++;
        it.second.to->connection_count++;
        for (auto &ju : {it.second.from, it.second.to}) {
            ju->layer.merge(it.second.layer);
        }
    }
    for (const auto &it : arcs) {
        it.second.from->connection_count++;
        it.second.to->connection_count++;
        it.second.center->connection_count++;
        for (auto &ju : {it.second.from, it.second.to}) {
            ju->layer.merge(it.second.layer);
        }
    }

    map_erase_if(junctions, [](const auto &it) { return it.second.connection_count == 0; });
}

void Package::update_warnings()
{
    warnings.clear();
    std::set<std::string> pad_names;
    for (const auto &it : pads) {
        auto x = pad_names.insert(it.second.name);
        if (!x.second) {
            warnings.emplace_back(it.second.placement.shift, "duplicate pad name");
        }
        for (auto p : it.second.pool_padstack->parameters_required) {
            if (it.second.parameter_set.count(p) == 0) {
                warnings.emplace_back(it.second.placement.shift, "missing parameter " + parameter_id_to_name(p));
            }
        }
    }
}

std::pair<Coordi, Coordi> Package::get_bbox() const
{
    Coordi a;
    Coordi b;
    for (const auto &it : pads) {
        auto bb_pad = it.second.placement.transform_bb(it.second.padstack.get_bbox());
        a = Coordi::min(a, bb_pad.first);
        b = Coordi::max(b, bb_pad.second);
    }
    for (const auto &it : polygons) {
        if (it.second.layer == BoardLayers::TOP_PACKAGE || it.second.layer == BoardLayers::BOTTOM_PACKAGE) {
            for (const auto &v : it.second.vertices) {
                a = Coordi::min(a, v.position);
                b = Coordi::max(b, v.position);
            }
        }
    }
    return std::make_pair(a, b);
}

static std::map<int, Layer> pkg_layers;

const std::map<int, Layer> &Package::get_layers() const
{
    if (pkg_layers.size() == 0) {
        auto add_layer = [](int n, bool r = false, bool c = false) {
            pkg_layers.emplace(std::piecewise_construct, std::forward_as_tuple(n),
                               std::forward_as_tuple(n, BoardLayers::get_layer_name(n), r, c));
        };
        add_layer(BoardLayers::L_OUTLINE);
        add_layer(BoardLayers::TOP_COURTYARD);
        add_layer(BoardLayers::TOP_ASSEMBLY);
        add_layer(BoardLayers::TOP_PACKAGE);
        add_layer(BoardLayers::TOP_PASTE);
        add_layer(BoardLayers::TOP_SILKSCREEN);
        add_layer(BoardLayers::TOP_MASK);
        add_layer(BoardLayers::TOP_COPPER, false, true);
        add_layer(BoardLayers::IN1_COPPER, false, true);
        add_layer(BoardLayers::BOTTOM_COPPER, true, true);
        add_layer(BoardLayers::BOTTOM_MASK, true);
        add_layer(BoardLayers::BOTTOM_SILKSCREEN, true);
        add_layer(BoardLayers::BOTTOM_PASTE);
        add_layer(BoardLayers::BOTTOM_PACKAGE);
        add_layer(BoardLayers::BOTTOM_ASSEMBLY, true);
        add_layer(BoardLayers::BOTTOM_COURTYARD);
    }
    return pkg_layers;
}

json Package::serialize() const
{
    json j;
    j["uuid"] = (std::string)uuid;
    j["type"] = "package";
    j["name"] = name;
    j["manufacturer"] = manufacturer;
    j["tags"] = tags;
    j["parameter_program"] = parameter_program.get_code();
    j["parameter_set"] = parameter_set_serialize(parameter_set);
    if (alternate_for && alternate_for->uuid != uuid)
        j["alternate_for"] = (std::string)alternate_for->uuid;
    j["models"] = json::object();
    for (const auto &it : models) {
        j["models"][(std::string)it.first] = it.second.serialize();
    }
    j["default_model"] = (std::string)default_model;
    j["junctions"] = json::object();
    for (const auto &it : junctions) {
        j["junctions"][(std::string)it.first] = it.second.serialize();
    }
    j["lines"] = json::object();
    for (const auto &it : lines) {
        j["lines"][(std::string)it.first] = it.second.serialize();
    }
    j["arcs"] = json::object();
    for (const auto &it : arcs) {
        j["arcs"][(std::string)it.first] = it.second.serialize();
    }
    j["texts"] = json::object();
    for (const auto &it : texts) {
        j["texts"][(std::string)it.first] = it.second.serialize();
    }
    j["pads"] = json::object();
    for (const auto &it : pads) {
        j["pads"][(std::string)it.first] = it.second.serialize();
    }
    j["polygons"] = json::object();
    for (const auto &it : polygons) {
        j["polygons"][(std::string)it.first] = it.second.serialize();
    }
    j["keepouts"] = json::object();
    for (const auto &it : keepouts) {
        j["keepouts"][(std::string)it.first] = it.second.serialize();
    }
    j["dimensions"] = json::object();
    for (const auto &it : dimensions) {
        j["dimensions"][(std::string)it.first] = it.second.serialize();
    }
    if (pictures.size()) {
        j["pictures"] = json::object();
        for (const auto &it : pictures) {
            j["pictures"][(std::string)it.first] = it.second.serialize();
        }
    }
    j["rules"] = rules.serialize();
    return j;
}

UUID Package::get_uuid() const
{
    return uuid;
}

const Package::Model *Package::get_model(const UUID &uu) const
{
    UUID uu2 = uu;
    if (uu2 == UUID()) {
        uu2 = default_model;
    }
    if (models.count(uu2)) {
        return &models.at(uu2);
    }
    else {
        return nullptr;
    }
}

int Package::get_max_pad_name() const
{
    std::vector<int> pad_nrs;
    for (const auto &it : pads) {
        try {
            int n = std::stoi(it.second.name);
            pad_nrs.push_back(n);
        }
        catch (...) {
        }
    }
    if (pad_nrs.size()) {
        int maxpad = *std::max_element(pad_nrs.begin(), pad_nrs.end());
        return maxpad;
    }
    return -1;
}

void Package::save_pictures(const std::string &dir) const
{
    pictures_save({&pictures}, dir, "pkg");
}

void Package::load_pictures(const std::string &dir)
{
    pictures_load({&pictures}, dir, "pkg");
}

} // namespace horizon
