#include "gerber_output_settings.hpp"
#include "board.hpp"
#include "board_layers.hpp"
#include "util/util.hpp"
#include "nlohmann/json.hpp"

namespace horizon {

const LutEnumStr<GerberOutputSettings::DrillMode> GerberOutputSettings::mode_lut = {
        {"merged", GerberOutputSettings::DrillMode::MERGED},
        {"individual", GerberOutputSettings::DrillMode::INDIVIDUAL},
};

GerberOutputSettings::GerberLayer::GerberLayer(int l) : layer(l)
{
    switch (layer) {
    case BoardLayers::L_OUTLINE:
        filename = ".gko";
        break;
    case BoardLayers::TOP_COPPER:
        filename = ".gtl";
        break;
    case BoardLayers::TOP_MASK:
        filename = ".gts";
        break;
    case BoardLayers::TOP_SILKSCREEN:
        filename = ".gto";
        break;
    case BoardLayers::TOP_PASTE:
        filename = ".gtp";
        break;
    case BoardLayers::BOTTOM_COPPER:
        filename = ".gbl";
        break;
    case BoardLayers::BOTTOM_MASK:
        filename = ".gbs";
        break;
    case BoardLayers::BOTTOM_SILKSCREEN:
        filename = ".gbo";
        break;
    case BoardLayers::BOTTOM_PASTE:
        filename = ".gbp";
        break;
    }
}

GerberOutputSettings::GerberLayer::GerberLayer(int l, const json &j)
    : layer(l), filename(j.at("filename").get<std::string>()), enabled(j.at("enabled"))
{
}

json GerberOutputSettings::GerberLayer::serialize() const
{
    json j;
    j["layer"] = layer;
    j["filename"] = filename;
    j["enabled"] = enabled;
    return j;
}

GerberOutputSettings::GerberOutputSettings(const json &j)
    : drill_pth_filename(j.at("drill_pth").get<std::string>()),
      drill_npth_filename(j.at("drill_npth").get<std::string>()), prefix(j.at("prefix").get<std::string>()),
      output_directory(j.at("output_directory").get<std::string>()), zip_output(j.value("zip_output", false))
{
    {
        const json &o = j["layers"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            int k = std::stoi(it.key());
            layers.emplace(std::piecewise_construct, std::forward_as_tuple(k), std::forward_as_tuple(k, it.value()));
        }
    }
    if (j.count("drill_mode")) {
        drill_mode = mode_lut.lookup(j.at("drill_mode"));
    }
    if (j.count("blind_buried_drills_filenames")) {
        const auto &o = j.at("blind_buried_drills_filenames");
        for (const auto &it : o) {
            blind_buried_drills_filenames.emplace(it.at("span"), it.at("filename").get<std::string>());
        }
    }
}

void GerberOutputSettings::update_for_board(const Board &brd)
{
    auto layers_from_board = brd.get_layers();
    // remove layers not on board
    map_erase_if(layers, [layers_from_board](const auto &it) { return layers_from_board.count(it.first) == 0; });

    // add new layers
    auto add_layer = [this](int l) { layers.emplace(l, l); };

    add_layer(BoardLayers::L_OUTLINE);
    add_layer(BoardLayers::TOP_PASTE);
    add_layer(BoardLayers::TOP_SILKSCREEN);
    add_layer(BoardLayers::TOP_MASK);
    add_layer(BoardLayers::TOP_COPPER);
    for (const auto &la : layers_from_board) {
        if (BoardLayers::is_copper(la.first) && la.first > BoardLayers::BOTTOM_COPPER
            && la.first < BoardLayers::TOP_COPPER)
            add_layer(la.first);
    }
    add_layer(BoardLayers::BOTTOM_COPPER);
    add_layer(BoardLayers::BOTTOM_MASK);
    add_layer(BoardLayers::BOTTOM_SILKSCREEN);
    add_layer(BoardLayers::BOTTOM_PASTE);
}

static std::string layer_to_string_for_drill(int l)
{
    switch (l) {
    case BoardLayers::TOP_COPPER:
        return "top";
    case BoardLayers::BOTTOM_COPPER:
        return "bottom";
    default:
        if (l < BoardLayers::TOP_COPPER && l > BoardLayers::BOTTOM_COPPER) {
            return "inner" + std::to_string(-l);
        }
    }
    return "?";
}

static std::string span_to_string_for_drill(const LayerRange &r)
{
    return "-" + layer_to_string_for_drill(r.end()) + "-" + layer_to_string_for_drill(r.start()) + ".txt";
}

void GerberOutputSettings::update_blind_buried_drills_filenames(const Board &brd)
{
    auto spans = brd.get_drill_spans();
    spans.erase(BoardLayers::layer_range_through);
    map_erase_if(blind_buried_drills_filenames, [&spans](const auto &it) { return spans.count(it.first) == 0; });
    for (auto &span : spans) {
        blind_buried_drills_filenames.emplace(span, span_to_string_for_drill(span));
    }
}

json GerberOutputSettings::serialize() const
{
    json j;
    j["drill_pth"] = drill_pth_filename;
    j["drill_npth"] = drill_npth_filename;
    j["prefix"] = prefix;
    j["output_directory"] = output_directory;
    j["zip_output"] = zip_output;
    j["drill_mode"] = mode_lut.lookup_reverse(drill_mode);
    j["layers"] = json::object();
    for (const auto &it : layers) {
        j["layers"][std::to_string(it.first)] = it.second.serialize();
    }
    if (blind_buried_drills_filenames.size()) {
        auto o = json::array();
        for (const auto &[span, filename] : blind_buried_drills_filenames) {
            o.push_back({{"span", span.serialize()}, {"filename", filename}});
        }
        j["blind_buried_drills_filenames"] = o;
    }


    return j;
}
} // namespace horizon
