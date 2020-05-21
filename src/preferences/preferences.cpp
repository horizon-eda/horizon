#include "preferences.hpp"
#include "common/lut.hpp"
#include "util/util.hpp"
#include <fstream>
#include <giomm/file.h>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>
#include "nlohmann/json.hpp"
#include "logger/logger.hpp"

namespace horizon {

static const LutEnumStr<Appearance::GridStyle> grid_style_lut = {
        {"cross", Appearance::GridStyle::CROSS},
        {"dot", Appearance::GridStyle::DOT},
        {"grid", Appearance::GridStyle::GRID},
};

static const LutEnumStr<Appearance::GridFineModifier> grid_fine_mod_lut = {
        {"alt", Appearance::GridFineModifier::ALT},
        {"ctrl", Appearance::GridFineModifier::CTRL},
};

static const LutEnumStr<Appearance::CursorSize> cursor_size_lut = {
        {"default", Appearance::CursorSize::DEFAULT},
        {"large", Appearance::CursorSize::LARGE},
        {"full", Appearance::CursorSize::FULL},
};

#ifdef G_OS_WIN32
static const bool capture_output_default = true;
#else
static const bool capture_output_default = false;
#endif

Preferences::Preferences()
{
    canvas_non_layer.appearance.layer_colors[0] = {1, 1, 0};
    capture_output = capture_output_default;
}

void Preferences::load_default()
{
    canvas_layer = CanvasPreferences();
    canvas_non_layer = CanvasPreferences();
    canvas_non_layer.appearance.layer_colors[0] = {1, 1, 0};
    key_sequences.load_from_json(json_from_resource("/org/horizon-eda/horizon/imp/keys_default.json"));
    capture_output = capture_output_default;
}

std::string Preferences::get_preferences_filename()
{
    return Glib::build_filename(get_config_dir(), "prefs.json");
}

#define COLORP_LUT_ITEM(x)                                                                                             \
    {                                                                                                                  \
#x, ColorP::x                                                                                                  \
    }

static const LutEnumStr<ColorP> colorp_lut = {
        COLORP_LUT_ITEM(FROM_LAYER),
        COLORP_LUT_ITEM(JUNCTION),
        COLORP_LUT_ITEM(FRAG_ORPHAN),
        COLORP_LUT_ITEM(AIRWIRE_ROUTER),
        COLORP_LUT_ITEM(TEXT_OVERLAY),
        COLORP_LUT_ITEM(HOLE),
        COLORP_LUT_ITEM(DIMENSION),
        COLORP_LUT_ITEM(ERROR),
        COLORP_LUT_ITEM(NET),
        COLORP_LUT_ITEM(BUS),
        COLORP_LUT_ITEM(FRAME),
        COLORP_LUT_ITEM(AIRWIRE),
        COLORP_LUT_ITEM(PIN),
        COLORP_LUT_ITEM(PIN_HIDDEN),
        COLORP_LUT_ITEM(DIFFPAIR),
        COLORP_LUT_ITEM(BACKGROUND),
        COLORP_LUT_ITEM(GRID),
        COLORP_LUT_ITEM(CURSOR_NORMAL),
        COLORP_LUT_ITEM(CURSOR_TARGET),
        COLORP_LUT_ITEM(ORIGIN),
        COLORP_LUT_ITEM(MARKER_BORDER),
        COLORP_LUT_ITEM(SELECTION_BOX),
        COLORP_LUT_ITEM(SELECTION_LINE),
        COLORP_LUT_ITEM(SELECTABLE_OUTER),
        COLORP_LUT_ITEM(SELECTABLE_INNER),
        COLORP_LUT_ITEM(SELECTABLE_PRELIGHT),
        COLORP_LUT_ITEM(SELECTABLE_ALWAYS),
        COLORP_LUT_ITEM(SEARCH),
        COLORP_LUT_ITEM(SEARCH_CURRENT),
        COLORP_LUT_ITEM(SHADOW),
        COLORP_LUT_ITEM(CONNECTION_LINE),
        COLORP_LUT_ITEM(NOPOPULATE_X),
};

json CanvasPreferences::serialize() const
{
    json j = serialize_colors();
    j["grid_style"] = grid_style_lut.lookup_reverse(appearance.grid_style);
    j["grid_opacity"] = appearance.grid_opacity;
    j["highlight_dim"] = appearance.highlight_dim;
    j["highlight_lighten"] = appearance.highlight_lighten;
    j["grid_fine_modifier"] = grid_fine_mod_lut.lookup_reverse(appearance.grid_fine_modifier);
    j["cursor_size"] = cursor_size_lut.lookup_reverse(appearance.cursor_size);
    j["cursor_size_tool"] = cursor_size_lut.lookup_reverse(appearance.cursor_size_tool);
    j["msaa"] = appearance.msaa;
    j["min_line_width"] = appearance.min_line_width;
    return j;
}

json CanvasPreferences::serialize_colors() const
{
    json j;
    json j_layer_colors = json::object();
    for (const auto &it : appearance.layer_colors) {
        j_layer_colors[std::to_string(it.first)] = color_to_json(it.second);
    }
    j["layer_colors"] = j_layer_colors;

    json j_colors = json::object();
    for (const auto &it : appearance.colors) {
        j_colors[colorp_lut.lookup_reverse(it.first)] = color_to_json(it.second);
    }
    j["colors"] = j_colors;
    return j;
}

void CanvasPreferences::load_colors_from_json(const json &j)
{
    if (j.count("layer_colors")) {
        const auto &o = j.at("layer_colors");
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            int l = std::stoi(it.key());
            appearance.layer_colors[l] = color_from_json(it.value());
        }
    }
    if (j.count("colors")) {
        const auto &o = j.at("colors");
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto c = colorp_lut.lookup(it.key(), ColorP::FROM_LAYER);
            if (c != ColorP::FROM_LAYER)
                appearance.colors[c] = color_from_json(it.value());
        }
    }
}

void CanvasPreferences::load_from_json(const json &j)
{
    appearance.grid_style = grid_style_lut.lookup(j.at("grid_style"));
    appearance.grid_opacity = j.value("grid_opacity", .4);
    appearance.highlight_dim = j.value("highlight_dim", .3);
    appearance.highlight_lighten = j.value("highlight_lighten", .3);
    appearance.grid_fine_modifier = grid_fine_mod_lut.lookup(j.value("grid_fine_modifier", "alt"));
    appearance.cursor_size = cursor_size_lut.lookup(j.value("cursor_size", "default"));
    appearance.cursor_size_tool = cursor_size_lut.lookup(j.value("cursor_size_tool", "default"));
    appearance.msaa = j.value("msaa", 0);
    appearance.min_line_width = j.value("min_line_width", 1.0);
    load_colors_from_json(j);
}

json SchematicPreferences::serialize() const
{
    json j;
    j["show_all_junctions"] = show_all_junctions;
    j["drag_start_net_line"] = drag_start_net_line;
    return j;
}

void SchematicPreferences::load_from_json(const json &j)
{
    show_all_junctions = j.value("show_all_junctions", false);
    drag_start_net_line = j.value("drag_start_net_line", true);
}

json BoardPreferences::serialize() const
{
    json j;
    j["drag_start_track"] = drag_start_track;
    j["highlight_on_top"] = highlight_on_top;
    j["show_text_in_tracks"] = show_text_in_tracks;
    j["show_text_in_vias"] = show_text_in_vias;
    return j;
}

void BoardPreferences::load_from_json(const json &j)
{
    drag_start_track = j.value("drag_start_track", true);
    highlight_on_top = j.value("highlight_on_top", true);
    show_text_in_tracks = j.value("show_text_in_tracks", true);
    show_text_in_vias = j.value("show_text_in_vias", true);
}

json ZoomPreferences::serialize() const
{
    json j;
    j["smooth_zoom_2d"] = smooth_zoom_2d;
    j["smooth_zoom_3d"] = smooth_zoom_3d;
    j["touchpad_pan"] = touchpad_pan;
    return j;
}

void ZoomPreferences::load_from_json(const json &j)
{
    smooth_zoom_2d = j.value("smooth_zoom_2d", true);
    smooth_zoom_3d = j.value("smooth_zoom_3d", false);
    touchpad_pan = j.value("touchpad_pan", false);
}

json KeySequencesPreferences::serialize() const
{
    json j;
    for (const auto &it : keys) {
        json k;
        auto a_str = action_lut.lookup_reverse(it.first.first);
        auto t_str = tool_lut.lookup_reverse(it.first.second);
        k["action"] = a_str;
        k["tool"] = t_str;
        k["keys"] = json::object();
        for (const auto &it2 : it.second) {
            k["keys"][std::to_string(static_cast<int>(it2.first))] = json::array();
            for (const auto &it3 : it2.second) {
                json seq;
                for (const auto &it4 : it3) {
                    json o;
                    o["key"] = gdk_keyval_name(it4.first);
                    o["mod"] = static_cast<int>(it4.second);
                    seq.push_back(o);
                }
                if (seq.size())
                    k["keys"][std::to_string(static_cast<int>(it2.first))].push_back(seq);
            }
        }
        j.push_back(k);
    }
    return j;
}

void KeySequencesPreferences::load_from_json(const json &j)
{
    keys.clear();
    append_from_json(j);
}

void KeySequencesPreferences::append_from_json(const json &j)
{
    for (const auto &it : j) {
        try {
            auto action = action_lut.lookup(it.at("action"), ActionID::NONE);
            if (action != ActionID::NONE) {
                auto tool = tool_lut.lookup(it.at("tool"));
                auto k = std::make_pair(action, tool);
                if (keys.count(k) == 0) {
                    keys[k];
                    auto &j2 = it.at("keys");
                    for (auto it2 = j2.cbegin(); it2 != j2.cend(); ++it2) {
                        auto av = static_cast<ActionCatalogItem::Availability>(std::stoi(it2.key()));
                        keys[k][av];
                        for (const auto &it3 : it2.value()) {
                            keys[k][av].emplace_back();
                            for (const auto &it4 : it3) {
                                std::string keyname = it4.at("key");
                                auto key = gdk_keyval_from_name(keyname.c_str());
                                auto mod = static_cast<GdkModifierType>(it4.at("mod").get<int>());
                                keys[k][av].back().emplace_back(key, mod);
                            }
                        }
                    }
                }
            }
        }
        catch (const std::exception &e) {
            Logger::log_warning("error loading key sequence", Logger::Domain::UNSPECIFIED, e.what());
        }
        catch (...) {
            Logger::log_warning("error loading key sequence", Logger::Domain::UNSPECIFIED, "unknown error");
        }
    }
}

void PartInfoPreferences::load_from_json(const json &j)
{
    enable = j.at("enable");
    url = j.at("url");
    preferred_distributor = j.at("preferred_distributor");
    ignore_moq_gt_1 = j.at("ignore_moq_gt_1");
    max_price_breaks = j.value("max_price_breaks", 3);
}

json PartInfoPreferences::serialize() const
{
    json j;
    j["enable"] = enable;
    j["url"] = url;
    j["preferred_distributor"] = preferred_distributor;
    j["ignore_moq_gt_1"] = ignore_moq_gt_1;
    j["max_price_breaks"] = max_price_breaks;
    return j;
}

bool PartInfoPreferences::is_enabled() const
{
    return enable && preferred_distributor.size();
}

json ActionBarPreferences::serialize() const
{
    json j;
    j["enable"] = enable;
    j["remember"] = remember;
    return j;
}

void ActionBarPreferences::load_from_json(const json &j)
{
    enable = j.value("enable", true);
    remember = j.value("remember", true);
}

json Preferences::serialize() const
{
    json j;
    j["canvas_layer"] = canvas_layer.serialize();
    j["canvas_non_layer"] = canvas_non_layer.serialize();
    j["schematic"] = schematic.serialize();
    j["key_sequences"] = key_sequences.serialize();
    j["board"] = board.serialize();
    j["zoom"] = zoom.serialize();
    j["capture_output"] = capture_output;
    j["partinfo"] = partinfo.serialize();
    j["action_bar"] = action_bar.serialize();
    return j;
}

void Preferences::save()
{
    std::string prefs_filename = get_preferences_filename();

    json j = serialize();
    save_json_to_file(prefs_filename, j);
}

void Preferences::load_from_json(const json &j)
{

    canvas_layer.load_from_json(j.at("canvas_layer"));
    canvas_non_layer.load_from_json(j.at("canvas_non_layer"));
    if (j.count("schematic"))
        schematic.load_from_json(j.at("schematic"));
    if (j.count("board"))
        board.load_from_json(j.at("board"));
    if (j.count("zoom"))
        zoom.load_from_json(j.at("zoom"));
    if (j.count("key_sequences"))
        key_sequences.load_from_json(j.at("key_sequences"));
    if (j.count("action_bar"))
        action_bar.load_from_json(j.at("action_bar"));
    key_sequences.append_from_json(json_from_resource("/org/horizon-eda/horizon/imp/keys_default.json"));
    capture_output = j.value("capture_output", capture_output_default);
    if (j.count("partinfo"))
        partinfo.load_from_json(j.at("partinfo"));
}

void Preferences::load()
{
    std::string prefs_filename = get_preferences_filename();
    std::string prefs_filename_old = Glib::build_filename(get_config_dir(), "imp-prefs.json");
    if (!Glib::file_test(prefs_filename, Glib::FILE_TEST_IS_REGULAR)) {
        if (Glib::file_test(prefs_filename_old, Glib::FILE_TEST_IS_REGULAR)) {
            json j = load_json_from_file(prefs_filename_old);
            load_from_json(j);
        }
        else {
            load_default();
        }
    }
    else {
        json j = load_json_from_file(prefs_filename);
        load_from_json(j);
    }
    s_signal_changed.emit();
}
} // namespace horizon
