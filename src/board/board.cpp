#include "board.hpp"
#include "board_layers.hpp"
#include "logger/log_util.hpp"
#include "logger/logger.hpp"
#include "pool/part.hpp"
#include "util/util.hpp"
#include "util/picture_load.hpp"
#include <list>
#include "nlohmann/json.hpp"
#include "pool/ipool.hpp"
#include "common/junction_util.hpp"
#include "util/bbox_accumulator.hpp"

namespace horizon {

Board::StackupLayer::StackupLayer(int l) : layer(l)
{
}
Board::StackupLayer::StackupLayer(int l, const json &j)
    : layer(l), thickness(j.at("thickness")), substrate_thickness(j.at("substrate_thickness"))
{
}

json Board::StackupLayer::serialize() const
{
    json j;
    j["thickness"] = thickness;
    j["substrate_thickness"] = substrate_thickness;
    return j;
}

BoardColors::BoardColors() : solder_mask({0, .5, 0}), silkscreen({1, 1, 1}), substrate({.2, .15, 0})
{
}

static const unsigned int app_version = 11;

unsigned int Board::get_app_version()
{
    return app_version;
}

Board::Board(const UUID &uu, const json &j, Block &iblock, IPool &pool)
    : uuid(uu), block(&iblock), name(j.at("name").get<std::string>()), version(app_version, j),
      n_inner_layers(j.value("n_inner_layers", 0))
{
    Logger::log_info("loading board " + (std::string)uuid, Logger::Domain::BOARD);
    if (j.count("stackup")) {
        const json &o = j["stackup"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            int l = std::stoi(it.key());
            stackup.emplace(std::piecewise_construct, std::forward_as_tuple(l), std::forward_as_tuple(l, it.value()));
        }
    }
    set_n_inner_layers(n_inner_layers);
    if (j.count("polygons")) {
        const json &o = j["polygons"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            load_and_log(polygons, ObjectType::POLYGON, std::forward_as_tuple(u, it.value()), Logger::Domain::BOARD);
        }
    }
    map_erase_if(polygons, [](const auto &a) { return a.second.vertices.size() == 0; });
    if (j.count("holes")) {
        const json &o = j["holes"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            load_and_log(holes, ObjectType::BOARD_HOLE, std::forward_as_tuple(u, it.value(), block, pool),
                         Logger::Domain::BOARD);
        }
    }
    std::set<UUID> texts_skip;
    if (j.count("packages")) {
        const json &o = j["packages"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            UUID comp_uuid(it.value().at("component").get<std::string>());
            auto u = UUID(it.key());
            if (block->components.count(comp_uuid) && block->components.at(comp_uuid).part != nullptr) {
                load_and_log(packages, ObjectType::BOARD_PACKAGE, std::forward_as_tuple(u, it.value(), *block, pool),
                             Logger::Domain::BOARD);
            }
            else {
                auto texts_lost = BoardPackage::peek_texts(it.value());
                std::copy(texts_lost.begin(), texts_lost.end(), std::inserter(texts_skip, texts_skip.end()));
                Logger::log_warning("not loading package " + (std::string)u + " since component is gone");
            }
        }
    }
    if (j.count("junctions")) {
        const json &o = j["junctions"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            load_and_log(junctions, ObjectType::JUNCTION, std::forward_as_tuple(u, it.value()), Logger::Domain::BOARD);
        }
    }
    if (j.count("tracks")) {
        const json &o = j["tracks"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            bool valid = true;
            for (const auto &it_ft : {it.value().at("from"), it.value().at("to")}) {
                if (it_ft.at("pad") != nullptr) {
                    UUIDPath<2> path(it_ft.at("pad").get<std::string>());
                    valid = packages.count(path.at(0));
                    if (valid) {
                        auto &pkg = packages.at(path.at(0));
                        if (pkg.component->part->pad_map.count(path.at(1)) == 0) {
                            valid = false;
                            break;
                        }
                    }
                    else {
                        break;
                    }
                }
            }
            if (valid) {
                load_and_log(tracks, ObjectType::TRACK, std::forward_as_tuple(u, it.value(), this),
                             Logger::Domain::BOARD);
            }
            else {
                Logger::log_warning("not loading track " + (std::string)u + " since at least one pad is gone");
            }
        }
    }
    if (j.count("vias")) {
        const json &o = j["vias"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            load_and_log(vias, ObjectType::VIA, std::forward_as_tuple(u, it.value(), pool, this),
                         Logger::Domain::BOARD);
        }
    }
    if (j.count("texts")) {
        const json &o = j["texts"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            if (!texts_skip.count(u))
                load_and_log(texts, ObjectType::TEXT, std::forward_as_tuple(u, it.value()), Logger::Domain::BOARD);
        }
    }
    if (j.count("lines")) {
        const json &o = j["lines"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            load_and_log(lines, ObjectType::LINE, std::forward_as_tuple(u, it.value(), *this), Logger::Domain::BOARD);
        }
    }
    if (j.count("planes")) {
        const json &o = j["planes"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            load_and_log(planes, ObjectType::PLANE, std::forward_as_tuple(u, it.value(), *this), Logger::Domain::BOARD);
        }
    }
    if (j.count("keepouts")) {
        const json &o = j["keepouts"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            load_and_log(keepouts, ObjectType::KEEPOUT, std::forward_as_tuple(u, it.value(), *this),
                         Logger::Domain::BOARD);
        }
    }
    if (j.count("dimensions")) {
        const json &o = j["dimensions"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            load_and_log(dimensions, ObjectType::DIMENSION, std::forward_as_tuple(u, it.value()),
                         Logger::Domain::BOARD);
        }
    }
    if (j.count("arcs")) {
        const json &o = j["arcs"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            load_and_log(arcs, ObjectType::ARC, std::forward_as_tuple(u, it.value(), *this), Logger::Domain::BOARD);
        }
    }
    if (j.count("connection_lines")) {
        const json &o = j["connection_lines"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            load_and_log(connection_lines, ObjectType::CONNECTION_LINE, std::forward_as_tuple(u, it.value(), this),
                         Logger::Domain::BOARD);
        }
    }
    if (j.count("included_boards")) {
        const json &o = j["included_boards"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            load_and_log(included_boards, ObjectType::BOARD, std::forward_as_tuple(u, it.value()),
                         Logger::Domain::BOARD);
        }
    }
    if (j.count("board_panels")) {
        const json &o = j["board_panels"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            load_and_log(board_panels, ObjectType::BOARD, std::forward_as_tuple(u, it.value(), *this),
                         Logger::Domain::BOARD);
        }
    }
    if (j.count("pictures")) {
        const json &o = j["pictures"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            load_and_log(pictures, ObjectType::PICTURE, std::forward_as_tuple(u, it.value()), Logger::Domain::BOARD);
        }
    }
    if (j.count("decals")) {
        const json &o = j["decals"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            load_and_log(decals, ObjectType::DECAL, std::forward_as_tuple(u, it.value(), pool, *this),
                         Logger::Domain::BOARD);
        }
    }
    if (j.count("rules")) {
        try {
            rules.load_from_json(j.at("rules"));
            rules.cleanup(block);
        }
        catch (const std::exception &e) {
            Logger::log_warning("couldn't load rules", Logger::Domain::BOARD, e.what());
        }
    }
    if (j.count("fab_output_settings")) {
        try {
            fab_output_settings = FabOutputSettings(j.at("fab_output_settings"));
        }
        catch (const std::exception &e) {
            Logger::log_warning("couldn't load fab output settings", Logger::Domain::BOARD, e.what());
        }
    }
    if (j.count("colors")) {
        try {
            const auto &o = j.at("colors");
            colors.solder_mask = color_from_json(o.at("solder_mask"));
            if (o.count("silkscreen")) {
                colors.silkscreen = color_from_json(o.at("silkscreen"));
            }
            colors.substrate = color_from_json(o.at("substrate"));
        }
        catch (const std::exception &e) {
            Logger::log_warning("couldn't colors", Logger::Domain::BOARD, e.what());
        }
    }
    if (j.count("pdf_export_settings")) {
        try {
            pdf_export_settings = PDFExportSettings(j.at("pdf_export_settings"));
        }
        catch (const std::exception &e) {
            Logger::log_warning("couldn't load pdf export settings", Logger::Domain::BOARD, e.what());
        }
    }
    if (j.count("step_export_settings")) {
        try {
            step_export_settings = STEPExportSettings(j.at("step_export_settings"));
        }
        catch (const std::exception &e) {
            Logger::log_warning("couldn't load step export settings", Logger::Domain::BOARD, e.what());
        }
    }
    if (j.count("pnp_export_settings")) {
        try {
            pnp_export_settings = PnPExportSettings(j.at("pnp_export_settings"));
        }
        catch (const std::exception &e) {
            Logger::log_warning("couldn't load p&p export settings", Logger::Domain::BOARD, e.what());
        }
    }
    if (j.count("grid_settings")) {
        try {
            grid_settings = GridSettings(j.at("grid_settings"));
        }
        catch (const std::exception &e) {
            Logger::log_warning("couldn't load grid settings", Logger::Domain::BOARD, e.what());
        }
    }
    fab_output_settings.update_for_board(*this);
    update_pdf_export_settings(pdf_export_settings);
    update_refs(); // fill in smashed texts
}

Board Board::new_from_file(const std::string &filename, Block &block, IPool &pool)
{
    auto j = load_json_from_file(filename);
    return Board(UUID(j.at("uuid").get<std::string>()), j, block, pool);
}


Board::Board(const UUID &uu, Block &bl) : uuid(uu), block(&bl), version(app_version)
{
    rules.add_rule(RuleID::CLEARANCE_COPPER);
    rules.add_rule(RuleID::CLEARANCE_COPPER_OTHER);
    rules.add_rule(RuleID::CLEARANCE_COPPER_KEEPOUT);
    rules.add_rule(RuleID::PLANE);
    {
        auto &r = rules.add_rule_t<RuleTrackWidth>();
        r.widths.emplace(std::piecewise_construct, std::forward_as_tuple(0), std::forward_as_tuple());
        // TBD: inner layers
        r.widths.emplace(std::piecewise_construct, std::forward_as_tuple(-100), std::forward_as_tuple());
    }
    {
        auto &r = rules.add_rule_t<RuleLayerPair>();
        r.layers = std::make_pair(BoardLayers::TOP_COPPER, BoardLayers::BOTTOM_COPPER);
    }
}

Junction *Board::get_junction(const UUID &uu)
{
    return &junctions.at(uu);
}

Polygon *Board::get_polygon(const UUID &uu)
{
    return &polygons.at(uu);
}

const std::map<int, Layer> &Board::get_layers() const
{
    return layers;
}

Board::Board(const Board &brd, CopyMode copy_mode)
    : layers(brd.layers), uuid(brd.uuid), block(brd.block), name(brd.name), polygons(brd.polygons), holes(brd.holes),
      junctions(brd.junctions), tracks(brd.tracks), texts(brd.texts), lines(brd.lines), arcs(brd.arcs),
      planes(brd.planes), keepouts(brd.keepouts), dimensions(brd.dimensions), connection_lines(brd.connection_lines),
      included_boards(brd.included_boards), board_panels(brd.board_panels), pictures(brd.pictures), decals(brd.decals),
      warnings(brd.warnings), rules(brd.rules), fab_output_settings(brd.fab_output_settings),
      grid_settings(brd.grid_settings), airwires(brd.airwires), stackup(brd.stackup), colors(brd.colors),
      pdf_export_settings(brd.pdf_export_settings), step_export_settings(brd.step_export_settings),
      pnp_export_settings(brd.pnp_export_settings), version(brd.version), n_inner_layers(brd.n_inner_layers)
{
    if (copy_mode == CopyMode::DEEP) {
        packages = brd.packages;
        vias = brd.vias;
    }
    else {
        for (const auto &it : brd.packages) {
            packages.emplace(std::piecewise_construct, std::forward_as_tuple(it.first),
                             std::forward_as_tuple(shallow_copy, it.second));
        }
        for (const auto &it : brd.vias) {
            vias.emplace(std::piecewise_construct, std::forward_as_tuple(it.first),
                         std::forward_as_tuple(shallow_copy, it.second));
        }
    }
    if (copy_mode == CopyMode::DEEP)
        update_refs();
}

Board::Board(const Board &brd) : Board(brd, CopyMode::DEEP)
{
}
Board::Board(shallow_copy_t sh, const Board &brd) : Board(brd, CopyMode::SHALLOW)
{
}

void Board::update_refs()
{
    for (auto &it : packages) {
        it.second.component.update(block->components);
        for (auto &it_pad : it.second.package.pads) {
            it_pad.second.net.update(block->nets);
        }
        for (auto &it_text : it.second.texts) {
            it_text.update(texts);
        }
    }
    for (auto &it : tracks) {
        it.second.update_refs(*this);
    }
    for (auto &it : airwires) {
        for (auto &it2 : it.second) {
            it2.update_refs(*this);
        }
    }
    for (auto &it : vias) {
        it.second.junction.update(junctions);
        it.second.net_set.update(block->nets);
    }
    for (auto &it : holes) {
        it.second.net.update(block->nets);
    }
    for (auto &it : junctions) {
        it.second.net.update(block->nets);
    }
    for (auto &it : lines) {
        auto &line = it.second;
        line.to = &junctions.at(line.to.uuid);
        line.from = &junctions.at(line.from.uuid);
    }
    for (auto &it : arcs) {
        auto &arc = it.second;
        arc.to.update(junctions);
        arc.from.update(junctions);
        arc.center.update(junctions);
    }
    for (auto &it : planes) {
        it.second.net.update(block->nets);
        it.second.polygon.update(polygons);
        it.second.polygon->usage = &it.second;
    }
    for (auto &it : keepouts) {
        it.second.polygon.update(polygons);
        it.second.polygon->usage = &it.second;
    }
    for (auto &it : connection_lines) {
        it.second.update_refs(*this);
    }
    for (auto &it : board_panels) {
        it.second.included_board.update(included_boards);
    }
}

void Board::update_junction_connections()
{
    for (auto &[uu, it] : junctions) {
        it.clear();
        it.connected_connection_lines.clear();
        it.connected_tracks.clear();
        it.connected_vias.clear();
        it.has_via = false;
        it.needs_via = false;
    }
    for (auto &[uu, it] : tracks) {
        for (const auto &it_ft : {it.from, it.to}) {
            if (it_ft.is_junc()) {
                auto &ju = *it_ft.junc;
                ju.connected_tracks.push_back(uu);
                ju.layer.merge(it.layer);
                if (ju.layer.is_multilayer())
                    ju.needs_via = true;
            }
        }
    }

    JunctionUtil::update(lines);
    JunctionUtil::update(arcs);

    for (auto &[uu, it] : connection_lines) {
        for (const auto &it_ft : {it.from, it.to}) {
            if (it_ft.is_junc())
                it_ft.junc->connected_connection_lines.push_back(uu);
        }
    }
    for (auto &[uu, it] : vias) {
        it.junction->has_via = true;
        it.junction->layer = LayerRange(BoardLayers::TOP_COPPER, BoardLayers::BOTTOM_COPPER);
        it.junction->connected_vias.push_back(uu);
    }
}

unsigned int Board::get_n_inner_layers() const
{
    return n_inner_layers;
}

void Board::set_n_inner_layers(unsigned int n)
{
    n_inner_layers = n;
    layers.clear();
    layers = {{200, {200, "Top Notes"}},
              {BoardLayers::OUTLINE_NOTES, {BoardLayers::OUTLINE_NOTES, "Outline Notes"}},
              {100, {100, "Outline"}},
              {60, {60, "Top Courtyard"}},
              {50, {50, "Top Assembly"}},
              {40, {40, "Top Package"}},
              {30, {30, "Top Paste"}},
              {20, {20, "Top Silkscreen"}},
              {10, {10, "Top Mask"}},
              {0, {0, "Top Copper", false, true}},
              {-100, {-100, "Bottom Copper", true, true}},
              {-110, {-110, "Bottom Mask", true}},
              {-120, {-120, "Bottom Silkscreen", true}},
              {-130, {-130, "Bottom Paste"}},
              {-140, {-140, "Bottom Package"}},
              {-150, {-150, "Bottom Assembly", true}},
              {-160, {-160, "Bottom Courtyard"}},
              {-200, {-200, "Bottom Notes", true}}};
    if (stackup.emplace(0, 0).second) { // if created
        stackup.at(0).substrate_thickness = 1.6_mm;
    }
    stackup.emplace(-100, -100);
    stackup.at(-100).substrate_thickness = 0;
    for (unsigned int i = 0; i < n_inner_layers; i++) {
        auto j = i + 1;
        layers.emplace(std::make_pair(-j, Layer(-j, "Inner " + std::to_string(j), false, true)));
        stackup.emplace(-j, -j);
    }
    map_erase_if(stackup, [this](const auto &x) { return layers.count(x.first) == 0; });
    fab_output_settings.update_for_board(*this);
    update_pdf_export_settings(pdf_export_settings);
}

void Board::update_pdf_export_settings(PDFExportSettings &settings)
{
    auto layers_from_board = get_layers();
    // remove layers not on board
    map_erase_if(settings.layers, [&layers_from_board](const auto &it) {
        return it.first != PDFExportSettings::HOLES_LAYER && layers_from_board.count(it.first) == 0;
    });

    // add new layers
    auto add_layer = [&settings](int l, bool enable = true) {
        settings.layers.emplace(
                std::piecewise_construct, std::forward_as_tuple(l),
                std::forward_as_tuple(l, Color(0, 0, 0), PDFExportSettings::Layer::Mode::OUTLINE, enable));
    };

    // add holes layer
    add_layer(PDFExportSettings::HOLES_LAYER, false);

    add_layer(BoardLayers::OUTLINE_NOTES);
    add_layer(BoardLayers::L_OUTLINE);
    add_layer(BoardLayers::TOP_PASTE, false);
    add_layer(BoardLayers::TOP_SILKSCREEN, false);
    add_layer(BoardLayers::TOP_MASK, false);
    add_layer(BoardLayers::TOP_MASK, false);
    add_layer(BoardLayers::TOP_ASSEMBLY);
    add_layer(BoardLayers::TOP_PACKAGE, false);
    add_layer(BoardLayers::TOP_COPPER, false);
    for (const auto &la : layers_from_board) {
        if (BoardLayers::is_copper(la.first) && la.first > BoardLayers::BOTTOM_COPPER
            && la.first < BoardLayers::TOP_COPPER)
            add_layer(la.first, false);
    }
    add_layer(BoardLayers::BOTTOM_COPPER, false);
    add_layer(BoardLayers::BOTTOM_MASK, false);
    add_layer(BoardLayers::BOTTOM_SILKSCREEN, false);
    add_layer(BoardLayers::BOTTOM_PASTE, false);
    add_layer(BoardLayers::BOTTOM_ASSEMBLY, false);
    add_layer(BoardLayers::BOTTOM_PACKAGE, false);
}

void Board::flip_package_layer(int &layer) const
{
    if (layer == BoardLayers::L_OUTLINE)
        return;
    else if (layer < 0 && layer > -100) {
        if (n_inner_layers == 0)
            return;
        if (-layer > (int)n_inner_layers)
            throw std::runtime_error("layer out of range");
        layer = -layer - (n_inner_layers + 1);
    }
    else {
        layer = -layer - 100;
    }
}

int Board::get_package_layer(bool flip, int layer) const
{
    if (flip)
        flip_package_layer(layer);
    return layer;
}

void Board::propagate_nets()
{
    // reset
    for (auto &it : tracks) {
        it.second.update_refs(*this);
        it.second.net = nullptr;
    }
    for (auto &it : junctions) {
        it.second.net = nullptr;
    }

    for (auto &it_via : vias) {
        auto ju = it_via.second.junction;
        if (it_via.second.net_set) {
            ju->net = it_via.second.net_set;
        }
    }

    bool assigned = true;
    std::set<const Track *> tracks_erase;
    while (assigned) {
        assigned = false;
        for (auto &it : tracks) {
            if (it.second.net) {
                for (const auto &it_ft : {it.second.from, it.second.to}) {
                    if (it_ft.is_pad()) {
                        if (it_ft.pad->net != it.second.net) { // connected to wrong pad, delete
                            tracks_erase.insert(&it.second);
                        }
                    }
                    else if (it_ft.is_junc()) {
                        if (it_ft.junc->net
                            && (it_ft.junc->net != it.second.net)) { // connected to wrong junction, delete
                            tracks_erase.insert(&it.second);
                        }
                        else if (it_ft.junc->net == nullptr) {
                            it_ft.junc->net = it.second.net;
                            assigned = true;
                        }
                    }
                }
            }
            else { // no net, take net from pad/junc
                for (const auto &it_ft : {it.second.from, it.second.to}) {
                    if (it_ft.is_pad()) {
                        if (it_ft.pad->net) {
                            it.second.net = it_ft.pad->net;
                            assigned = true;
                        }
                    }
                    else if (it_ft.is_junc()) {
                        if (it_ft.junc->net) {
                            it.second.net = it_ft.junc->net;
                            assigned = true;
                        }
                    }
                }
            }
        }
    }
    if (tracks_erase.size())
        map_erase_if(tracks, [&tracks_erase](const auto &it) { return tracks_erase.count(&it.second); });

    map_erase_if(connection_lines, [](auto &it) {
        ConnectionLine &li = it.second;
        return (li.from.get_net() && li.to.get_net());
    });
}

void Board::vacuum_junctions()
{
    map_erase_if(junctions, [](auto &x) {
        const BoardJunction &ju = x.second;
        return (ju.connected_lines.size() == 0) && (ju.connected_arcs.size() == 0) && (ju.connected_tracks.size() == 0)
               && (ju.connected_vias.size() == 0);
    });
}


static std::string replace_text_map(const std::string &s, const std::map<std::string, std::string> &m)
{
    auto fn = [&m](const auto &k) -> std::optional<std::string> {
        if (m.count(k))
            return m.at(k);
        else
            return {};
    };
    return interpolate_text(s, fn);
}

void Board::expand()
{
    expand_flags = EXPAND_ALL;
    expand_some();
}

void Board::expand_some()
{
    delete_dependants();
    warnings.clear();

    for (auto &it : texts) {
        it.second.overridden = false;
    }

    if (expand_flags & EXPAND_PACKAGES)
        expand_packages();

    for (auto &[uu, pkg] : packages) {
        pkg.update_texts(*this);
    }

    update_junction_connections();

    for (const auto &it : tracks) {

        if (it.second.from.get_position() == it.second.to.get_position()) {
            warnings.emplace_back(it.second.from.get_position(), "Zero length track");
        }
    }

    auto params = rules.get_parameters();
    if (expand_flags & EXPAND_VIAS) {
        for (auto &it : vias) {
            it.second.expand(*this);
        }
    }
    for (auto &it : holes) {
        it.second.padstack = *it.second.pool_padstack;
        ParameterSet ps_hole = it.second.parameter_set;
        ps_hole.emplace(ParameterID::HOLE_SOLDER_MASK_EXPANSION, params->hole_solder_mask_expansion);
        it.second.padstack.apply_parameter_set(ps_hole);
        it.second.padstack.expand_inner(n_inner_layers);
        if (it.second.padstack.type == Padstack::Type::HOLE && it.second.net == nullptr) {
            warnings.emplace_back(it.second.placement.shift, "PTH hole without net");
        }
        if (it.second.padstack.type != Padstack::Type::HOLE && it.second.padstack.type != Padstack::Type::MECHANICAL) {
            warnings.emplace_back(it.second.placement.shift, "Illegal padstack type");
        }
        for (auto p : it.second.pool_padstack->parameters_required) {
            if (it.second.parameter_set.count(p) == 0) {
                warnings.emplace_back(it.second.placement.shift, "missing parameter " + parameter_id_to_name(p));
            }
        }
    }

    for (const auto &it : junctions) {
        if (it.second.needs_via && !it.second.has_via) {
            warnings.emplace_back(it.second.position, "Junction needs via");
        }
    }

    vacuum_junctions();
    delete_dependants(); // deletes connection lines

    for (auto &it : polygons) {
        it.second.usage = nullptr;
    }
    for (auto &it : planes) {
        it.second.polygon->usage = &it.second;
    }
    for (auto &it : keepouts) {
        it.second.polygon->usage = &it.second;
    }

    if (expand_flags & EXPAND_PROPAGATE_NETS) {
        propagate_nets();
    }

    for (auto &it : vias) {
        if (it.second.junction->net == nullptr && !it.second.net_set) {
            warnings.emplace_back(it.second.junction->position, "Via without net");
        }
        else if (it.second.junction->net && it.second.net_set && (it.second.junction->net != it.second.net_set)) {
            warnings.emplace_back(it.second.junction->position, "Via net mismatch");
        }
    }

    for (auto &it : texts) {
        if (it.second.overridden == false) {
            if (std::count(it.second.text.begin(), it.second.text.end(), '$')) {
                it.second.overridden = true;
                it.second.text_override = replace_text_map(it.second.text, block->project_meta);
            }
        }
    }

    if (expand_flags & EXPAND_ALL_AIRWIRES)
        update_all_airwires();
    else if (expand_flags & EXPAND_AIRWIRES)
        update_airwires(false, airwires_expand);

    expand_flags = EXPAND_NONE;
    airwires_expand.clear();
}

ParameterSet Board::get_parameters() const
{
    auto params = rules.get_parameters();
    ParameterSet pset = {
            {ParameterID::COURTYARD_EXPANSION, params->courtyard_expansion},
            {ParameterID::PASTE_MASK_CONTRACTION, params->paste_mask_contraction},
            {ParameterID::SOLDER_MASK_EXPANSION, params->solder_mask_expansion},
            {ParameterID::HOLE_SOLDER_MASK_EXPANSION, params->hole_solder_mask_expansion},
    };
    return pset;
}

void Board::expand_packages()
{
    const auto pset = get_parameters();

    for (auto &it : packages) {
        if (it.second.update_package(*this) == false)
            warnings.emplace_back(it.second.placement.shift, "Incompatible alt pkg");
        if (auto err = it.second.package.apply_parameter_set(pset)) {
            Logger::log_critical("Package " + it.second.component->refdes + " parameter program failed",
                                 Logger::Domain::BOARD, err.value());
        }
    }

    update_refs();

    // assign nets to pads based on netlist
    for (auto &it : packages) {
        it.second.update_nets();
    }
}

void Board::disconnect_package(BoardPackage *pkg)
{
    std::map<Pad *, BoardJunction *> pad_junctions;
    for (auto &it_track : tracks) {
        Track *tr = &it_track.second;
        // if((line->to_symbol && line->to_symbol->uuid == it.uuid) ||
        // (line->from_symbol &&line->from_symbol->uuid == it.uuid)) {
        for (auto it_ft : {&tr->to, &tr->from}) {
            if (it_ft->package == pkg) {
                BoardJunction *j = nullptr;
                if (pad_junctions.count(it_ft->pad)) {
                    j = pad_junctions.at(it_ft->pad);
                }
                else {
                    auto uu = UUID::random();
                    auto x = pad_junctions.emplace(it_ft->pad, &junctions.emplace(uu, uu).first->second);
                    j = x.first->second;
                }
                auto c = it_ft->get_position();
                j->position = c;
                j->net = it_ft->pad->net;
                it_ft->connect(j);
            }
        }
    }
}

void Board::smash_package(BoardPackage *pkg)
{
    if (pkg->smashed)
        return;
    pkg->smashed = true;
    const Package *ppkg;

    if (pkg->alternate_package)
        ppkg = pkg->alternate_package;
    else
        ppkg = pkg->pool_package;

    for (const auto &it : ppkg->texts) {
        if (BoardLayers::is_silkscreen(it.second.layer)) { // top or bottom silkscreen
            auto uu = UUID::random();
            auto &x = texts.emplace(uu, uu).first->second;
            x.from_smash = true;
            x.overridden = true;
            x.placement = pkg->placement;
            if (x.placement.mirror) {
                x.placement.invert_angle();
            }
            x.placement.accumulate(it.second.placement);
            x.text = it.second.text;
            x.layer = it.second.layer;
            if (pkg->flip)
                flip_package_layer(x.layer);

            x.size = it.second.size;
            x.width = it.second.width;
            pkg->texts.push_back(&x);
        }
    }
}

void Board::copy_package_silkscreen_texts(BoardPackage *dest, const BoardPackage *src)
{
    // Copy smash status
    unsmash_package(dest);
    if (!src->smashed) {
        return;
    }
    dest->smashed = true;

    // Copy all texts from src
    for (const auto &it : src->texts) {
        if (!BoardLayers::is_silkscreen(it->layer)) {
            // Not on top or bottom silkscreen
            continue;
        }

        auto uu = UUID::random();
        auto &x = texts.emplace(uu, uu).first->second;
        x.from_smash = true;
        x.overridden = true;

        x.placement = dest->placement;
        Placement rp = it->placement;
        Placement ref_placement = src->placement;
        if (ref_placement.mirror) {
            ref_placement.invert_angle();
        }
        rp.make_relative(ref_placement);
        if (x.placement.mirror) {
            x.placement.invert_angle();
        }
        x.placement.accumulate(rp);

        x.text = it->text;
        x.layer = it->layer;
        if (src->flip != dest->flip) {
            flip_package_layer(x.layer);
        }

        x.size = it->size;
        x.width = it->width;
        dest->texts.push_back(&x);
    }
}

void Board::smash_package_silkscreen_graphics(BoardPackage *pkg)
{
    if (pkg->omit_silkscreen)
        return;

    std::map<Junction *, Junction *> junction_xlat;
    auto tr = pkg->placement;
    if (pkg->flip)
        tr.invert_angle();
    auto get_junction = [&junction_xlat, this, tr](Junction *ju) -> Junction * {
        if (junction_xlat.count(ju)) {
            return junction_xlat.at(ju);
        }
        else {
            auto uu = UUID::random();
            auto &new_junction = junctions.emplace(uu, uu).first->second;
            new_junction.position = tr.transform(ju->position);
            junction_xlat.emplace(ju, &new_junction);
            return &new_junction;
        }
    };

    for (const auto &it : pkg->package.lines) {
        if (BoardLayers::is_silkscreen(it.second.layer)) {
            auto uu = UUID::random();
            auto &new_line = lines.emplace(uu, uu).first->second;
            new_line.from = get_junction(it.second.from);
            new_line.to = get_junction(it.second.to);
            new_line.width = it.second.width;
            new_line.layer = it.second.layer;
        }
    }
    for (const auto &it : pkg->package.arcs) {
        if (BoardLayers::is_silkscreen(it.second.layer)) {
            auto uu = UUID::random();
            auto &new_arc = arcs.emplace(uu, uu).first->second;
            new_arc.from = get_junction(it.second.from);
            new_arc.to = get_junction(it.second.to);
            new_arc.center = get_junction(it.second.center);
            new_arc.width = it.second.width;
            new_arc.layer = it.second.layer;
        }
    }
    pkg->omit_silkscreen = true;
}

void Board::smash_panel_outline(BoardPanel &panel)
{
    if (panel.omit_outline)
        return;

    for (const auto &it : panel.included_board->board->polygons) {
        if (it.second.layer == BoardLayers::L_OUTLINE) {
            auto uu = UUID::random();
            auto &new_poly = polygons.emplace(uu, uu).first->second;
            new_poly.layer = BoardLayers::L_OUTLINE;
            for (const auto &it_v : it.second.vertices) {
                new_poly.vertices.emplace_back();
                auto &v = new_poly.vertices.back();
                v.arc_center = panel.placement.transform(it_v.arc_center);
                v.arc_reverse = it_v.arc_reverse;
                v.type = it_v.type;
                v.position = panel.placement.transform(it_v.position);
            }
        }
    }
    panel.omit_outline = true;
}

void Board::smash_package_outline(BoardPackage &pkg)
{
    if (pkg.omit_outline)
        return;
    auto tr = pkg.placement;
    if (pkg.flip)
        tr.invert_angle();

    for (const auto &it : pkg.package.polygons) {
        if (it.second.layer == BoardLayers::L_OUTLINE) {
            auto uu = UUID::random();
            auto &new_poly = polygons.emplace(uu, uu).first->second;
            new_poly.layer = BoardLayers::L_OUTLINE;
            for (const auto &it_v : it.second.vertices) {
                new_poly.vertices.emplace_back();
                auto &v = new_poly.vertices.back();
                v.arc_center = tr.transform(it_v.arc_center);
                v.arc_reverse = it_v.arc_reverse;
                v.type = it_v.type;
                v.position = tr.transform(it_v.position);
            }
        }
    }
    pkg.omit_outline = true;
}

void Board::unsmash_package(BoardPackage *pkg)
{
    if (!pkg->smashed)
        return;
    pkg->smashed = false;
    for (auto &it : pkg->texts) {
        if (it->from_smash) {
            texts.erase(it->uuid); // expand will delete from sym->texts
        }
    }
}

void Board::delete_dependants()
{
    map_erase_if(vias, [this](auto &it) { return junctions.count(it.second.junction.uuid) == 0; });
    map_erase_if(connection_lines, [this](auto &it) {
        for (const auto &it_ft : {it.second.from, it.second.to}) {
            if (it_ft.is_junc() && junctions.count(it_ft.junc.uuid) == 0)
                return true;
        }
        return false;
    });
    map_erase_if(planes, [this](auto &it) { return polygons.count(it.second.polygon.uuid) == 0; });
    map_erase_if(keepouts, [this](auto &it) { return polygons.count(it.second.polygon.uuid) == 0; });
}

std::vector<KeepoutContour> Board::get_keepout_contours() const
{
    std::vector<KeepoutContour> r;
    for (const auto &it : keepouts) {
        r.emplace_back();
        ClipperLib::Path &contour = r.back().contour;
        r.back().keepout = &it.second;
        auto poly2 = it.second.polygon->remove_arcs();
        contour.reserve(poly2.vertices.size());
        for (const auto &itv : poly2.vertices) {
            contour.push_back({itv.position.x, itv.position.y});
        }
    }
    for (const auto &it_pkg : packages) {
        Placement tr = it_pkg.second.placement;
        if (it_pkg.second.flip)
            tr.invert_angle();
        for (const auto &it : it_pkg.second.package.keepouts) {
            r.emplace_back();
            ClipperLib::Path &contour = r.back().contour;
            r.back().keepout = &it.second;
            r.back().pkg = &it_pkg.second;
            auto poly2 = it.second.polygon->remove_arcs();
            contour.reserve(poly2.vertices.size());
            for (const auto &itv : poly2.vertices) {
                auto p = tr.transform(itv.position);
                contour.push_back({p.x, p.y});
            }
        }
    }
    return r;
}
std::pair<Coordi, Coordi> Board::get_bbox() const
{
    BBoxAccumulator<Coordi::type> acc;
    for (const auto &it : polygons) {
        if (it.second.layer == BoardLayers::L_OUTLINE) { // outline
            acc.accumulate(it.second.get_bbox());
        }
    }
    for (const auto &[_, package] : packages) {
        acc.accumulate(package.get_bbox());
    }

    return acc.get_or({{-10_mm, -10_mm}, {10_mm, 10_mm}});
}

std::map<const BoardPackage *, PnPRow> Board::get_PnP(const PnPExportSettings &settings) const
{
    std::map<const BoardPackage *, PnPRow> r;
    for (const auto &it : packages) {
        if ((it.second.component->nopopulate && !settings.include_nopopulate)
            || it.second.component->part->get_flag(Part::Flag::EXCLUDE_PNP)) {
            continue;
        }

        PnPRow row;
        row.refdes = it.second.component->refdes;
        row.package = it.second.package.name;
        row.MPN = it.second.component->part->get_MPN();
        row.value = it.second.component->part->get_value();
        row.manufacturer = it.second.component->part->get_manufacturer();
        row.side = it.second.flip ? PnPRow::Side::BOTTOM : PnPRow::Side::TOP;
        row.placement = it.second.placement;
        if (it.second.flip)
            row.placement.inc_angle_deg(180);
        row.placement.mirror = false;
        r.emplace(&it.second, row);
    }
    return r;
}

json Board::serialize() const
{
    json j;
    version.serialize(j);
    j["type"] = "board";
    j["uuid"] = (std::string)uuid;
    j["block"] = (std::string)block->uuid;
    j["name"] = name;
    j["n_inner_layers"] = n_inner_layers;
    j["rules"] = rules.serialize();
    j["fab_output_settings"] = fab_output_settings.serialize();
    {
        json o;
        o["solder_mask"] = color_to_json(colors.solder_mask);
        o["silkscreen"] = color_to_json(colors.silkscreen);
        o["substrate"] = color_to_json(colors.substrate);
        j["colors"] = o;
    }
    j["pdf_export_settings"] = pdf_export_settings.serialize_board();
    j["step_export_settings"] = step_export_settings.serialize();
    j["pnp_export_settings"] = pnp_export_settings.serialize();
    j["grid_settings"] = grid_settings.serialize();
    j["polygons"] = json::object();
    for (const auto &it : polygons) {
        j["polygons"][(std::string)it.first] = it.second.serialize();
    }
    j["holes"] = json::object();
    for (const auto &it : holes) {
        j["holes"][(std::string)it.first] = it.second.serialize();
    }
    j["packages"] = json::object();
    for (const auto &it : packages) {
        j["packages"][(std::string)it.first] = it.second.serialize();
    }
    j["junctions"] = json::object();
    for (const auto &it : junctions) {
        j["junctions"][(std::string)it.first] = it.second.serialize();
    }
    j["tracks"] = json::object();
    for (const auto &it : tracks) {
        j["tracks"][(std::string)it.first] = it.second.serialize();
    }
    j["vias"] = json::object();
    for (const auto &it : vias) {
        j["vias"][(std::string)it.first] = it.second.serialize();
    }
    j["texts"] = json::object();
    for (const auto &it : texts) {
        j["texts"][(std::string)it.first] = it.second.serialize();
    }
    j["lines"] = json::object();
    for (const auto &it : lines) {
        j["lines"][(std::string)it.first] = it.second.serialize();
    }
    j["arcs"] = json::object();
    for (const auto &it : arcs) {
        j["arcs"][(std::string)it.first] = it.second.serialize();
    }
    j["planes"] = json::object();
    for (const auto &it : planes) {
        j["planes"][(std::string)it.first] = it.second.serialize();
    }
    j["stackup"] = json::object();
    for (const auto &it : stackup) {
        j["stackup"][std::to_string(it.first)] = it.second.serialize();
    }
    j["dimensions"] = json::object();
    for (const auto &it : dimensions) {
        j["dimensions"][(std::string)it.first] = it.second.serialize();
    }
    j["keepouts"] = json::object();
    for (const auto &it : keepouts) {
        j["keepouts"][(std::string)it.first] = it.second.serialize();
    }
    j["connection_lines"] = json::object();
    for (const auto &it : connection_lines) {
        j["connection_lines"][(std::string)it.first] = it.second.serialize();
    }
    if (included_boards.size()) {
        j["included_boards"] = json::object();
        for (const auto &it : included_boards) {
            j["included_boards"][(std::string)it.first] = it.second.serialize();
        }
    }
    if (board_panels.size()) {
        j["board_panels"] = json::object();
        for (const auto &it : board_panels) {
            j["board_panels"][(std::string)it.first] = it.second.serialize();
        }
    }
    if (pictures.size()) {
        j["pictures"] = json::object();
        for (const auto &it : pictures) {
            j["pictures"][(std::string)it.first] = it.second.serialize();
        }
    }
    if (decals.size()) {
        j["decals"] = json::object();
        for (const auto &it : decals) {
            j["decals"][(std::string)it.first] = it.second.serialize();
        }
    }
    return j;
}

void Board::save_pictures(const std::string &dir) const
{
    pictures_save({&pictures}, dir, "brd");
}

void Board::load_pictures(const std::string &dir)
{
    pictures_load({&pictures}, dir, "brd");
}

ItemSet Board::get_pool_items_used() const
{
    ItemSet items_needed;

    for (const auto &it_pkg : packages) {
        // don't use pool_package because of alternate pkg
        items_needed.emplace(ObjectType::PACKAGE, it_pkg.second.package.uuid);
        for (const auto &it_pad : it_pkg.second.package.pads) {
            items_needed.emplace(ObjectType::PADSTACK, it_pad.second.pool_padstack->uuid);
        }
    }
    for (const auto &it_via : vias) {
        items_needed.emplace(ObjectType::PADSTACK, it_via.second.pool_padstack->uuid);
    }
    for (const auto &it_hole : holes) {
        items_needed.emplace(ObjectType::PADSTACK, it_hole.second.pool_padstack->uuid);
    }
    for (const auto &it_decal : decals) {
        items_needed.emplace(ObjectType::DECAL, it_decal.second.get_decal().uuid);
    }
    return items_needed;
}

} // namespace horizon
