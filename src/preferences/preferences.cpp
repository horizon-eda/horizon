#include "preferences.hpp"
#include "common/lut.hpp"
#include "util/util.hpp"
#include <fstream>
#include <giomm/file.h>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>
#include "nlohmann/json.hpp"
#include "logger/logger.hpp"
#include "imp/in_tool_action_catalog.hpp"
#include "core/tool_id.hpp"
#include "imp/actions.hpp"

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

static const LutEnumStr<Preferences::StockInfoProviderSel> stock_info_provider_lut = {
        {"none", Preferences::StockInfoProviderSel::NONE},
        {"partinfo", Preferences::StockInfoProviderSel::PARTINFO},
        {"digikey", Preferences::StockInfoProviderSel::DIGIKEY}};

static const LutEnumStr<ShowViaSpan> show_via_span_lut = {
        {"none", ShowViaSpan::NONE},
        {"blind_buried", ShowViaSpan::BLIND_BURIED},
        {"all", ShowViaSpan::ALL},
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
    in_tool_key_sequences.load_from_json(json_from_resource("/org/horizon-eda/horizon/imp/in_tool_keys_default.json"));
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
        COLORP_LUT_ITEM(PROJECTION),
        COLORP_LUT_ITEM(NET_TIE),
        COLORP_LUT_ITEM(SYMBOL_BOUNDING_BOX),
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
    j["min_selectable_size"] = appearance.min_selectable_size;
    j["snap_radius"] = appearance.snap_radius;
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
    appearance.min_selectable_size = j.value("min_selectable_size", 20.0);
    appearance.snap_radius = j.value("snap_radius", 30.0);
    load_colors_from_json(j);
}

json SchematicPreferences::serialize() const
{
    json j;
    j["show_all_junctions"] = show_all_junctions;
    j["drag_start_net_line"] = drag_start_net_line;
    j["bend_non_ortho"] = bend_non_ortho;
    return j;
}

void SchematicPreferences::load_from_json(const json &j)
{
    show_all_junctions = j.value("show_all_junctions", false);
    drag_start_net_line = j.value("drag_start_net_line", true);
    bend_non_ortho = j.value("bend_non_ortho", true);
}

json BoardPreferences::serialize() const
{
    json j;
    j["drag_start_track"] = drag_start_track;
    j["highlight_on_top"] = highlight_on_top;
    j["show_text_in_tracks"] = show_text_in_tracks;
    j["show_text_in_vias"] = show_text_in_vias;
    j["show_via_span"] = show_via_span_lut.lookup_reverse(show_via_span);
    j["move_using_router"] = move_using_router;
    return j;
}

void BoardPreferences::load_from_json(const json &j)
{
    drag_start_track = j.value("drag_start_track", true);
    highlight_on_top = j.value("highlight_on_top", true);
    show_text_in_tracks = j.value("show_text_in_tracks", true);
    show_text_in_vias = j.value("show_text_in_vias", true);
    show_via_span = show_via_span_lut.lookup(j.value("show_via_span", ""), ShowViaSpan::BLIND_BURIED);
    move_using_router = j.value("move_using_router", true);
}

json ZoomPreferences::serialize() const
{
    json j;
    j["smooth_zoom_2d"] = smooth_zoom_2d;
    j["smooth_zoom_3d"] = smooth_zoom_3d;
    j["touchpad_pan"] = touchpad_pan;
    j["zoom_factor"] = zoom_factor;
    j["keyboard_zoom_to_cursor"] = keyboard_zoom_to_cursor;
    return j;
}

void ZoomPreferences::load_from_json(const json &j)
{
    smooth_zoom_2d = j.value("smooth_zoom_2d", true);
    smooth_zoom_3d = j.value("smooth_zoom_3d", false);
    touchpad_pan = j.value("touchpad_pan", false);
    zoom_factor = j.value("zoom_factor", 50);
    keyboard_zoom_to_cursor = j.value("keyboard_zoom_to_cursor", false);
}

json KeySequencesPreferences::serialize() const
{
    json j;
    for (const auto &it : keys) {
        json k;
        auto a_str = action_lut.lookup_reverse(it.first.action);
        auto t_str = tool_lut.lookup_reverse(it.first.tool);
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
                auto tool = tool_lut.lookup(it.at("tool"), ToolID::NONE);
                if (action != ActionID::TOOL || (action == ActionID::TOOL && tool != ToolID::NONE)) {
                    auto k = ActionToolID(action, tool);
                    if (keys.count(k) == 0) {
                        keys[k];
                        for (const auto &[av_str, value] : it.at("keys").items()) {
                            const auto av = static_cast<ActionCatalogItem::Availability>(std::stoi(av_str));
                            keys[k][av];
                            for (const auto &it3 : value) {
                                keys[k][av].emplace_back();
                                for (const auto &it4 : it3) {
                                    const auto keyname = it4.at("key").get<std::string>();
                                    const auto key = gdk_keyval_from_name(keyname.c_str());
                                    const auto mod = static_cast<GdkModifierType>(it4.at("mod").get<int>());
                                    keys[k][av].back().emplace_back(key, mod);
                                }
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
json InToolKeySequencesPreferences::serialize() const
{
    json j;
    for (const auto &[action, sequences] : keys) {
        auto a_str = in_tool_action_lut.lookup_reverse(action);
        for (const auto &it2 : sequences) {
            json seq;
            for (const auto &it4 : it2) {
                json o;
                o["key"] = gdk_keyval_name(it4.first);
                o["mod"] = static_cast<int>(it4.second);
                seq.push_back(o);
            }
            if (seq.size())
                j[a_str].push_back(seq);
        }
    }
    return j;
}

void InToolKeySequencesPreferences::load_from_json(const json &j)
{
    keys.clear();
    append_from_json(j);
}

void InToolKeySequencesPreferences::append_from_json(const json &j)
{
    for (const auto &[a_str, keys_seqs] : j.items()) {
        try {
            auto action = in_tool_action_lut.lookup(a_str, InToolActionID::NONE);
            if (action != InToolActionID::NONE) {
                if (keys.count(action) == 0) {
                    for (const auto &seq : keys_seqs) {
                        keys[action].emplace_back();
                        for (const auto &it4 : seq) {
                            const auto keyname = it4.at("key").get<std::string>();
                            const auto key = gdk_keyval_from_name(keyname.c_str());
                            const auto mod = static_cast<GdkModifierType>(it4.at("mod").get<int>());
                            keys[action].back().emplace_back(key, mod);
                        }
                    }
                }
            }
        }
        catch (const std::exception &e) {
            Logger::log_warning("error loading in-tool key sequence", Logger::Domain::UNSPECIFIED, e.what());
        }
        catch (...) {
            Logger::log_warning("error loading int-tool key sequence", Logger::Domain::UNSPECIFIED, "unknown error");
        }
    }
}

void PartInfoPreferences::load_from_json(const json &j)
{
    url = j.at("url").get<std::string>();
    preferred_distributor = j.at("preferred_distributor").get<std::string>();
    ignore_moq_gt_1 = j.at("ignore_moq_gt_1").get<bool>();
    max_price_breaks = j.value("max_price_breaks", 3);
}

json PartInfoPreferences::serialize() const
{
    json j;
    j["enable"] = false;
    j["url"] = url;
    j["preferred_distributor"] = preferred_distributor;
    j["ignore_moq_gt_1"] = ignore_moq_gt_1;
    j["max_price_breaks"] = max_price_breaks;
    return j;
}

void DigiKeyApiPreferences::load_from_json(const json &j)
{
    client_id = j.at("client_id").get<std::string>();
    client_secret = j.at("client_secret").get<std::string>();
    currency = j.value("currency", "EUR");
    site = j.value("site", "DE");
    max_price_breaks = j.value("max_price_breaks", 3);
}

json DigiKeyApiPreferences::serialize() const
{
    json j;
    j["enable"] = false;
    j["client_id"] = client_id;
    j["client_secret"] = client_secret;
    j["site"] = site;
    j["currency"] = currency;
    j["max_price_breaks"] = max_price_breaks;
    return j;
}

json ActionBarPreferences::serialize() const
{
    json j;
    j["enable"] = enable;
    j["remember"] = remember;
    j["show_in_tool"] = show_in_tool;
    return j;
}

void ActionBarPreferences::load_from_json(const json &j)
{
    enable = j.value("enable", true);
    remember = j.value("remember", true);
    show_in_tool = j.value("show_in_tool", true);
}

json MousePreferences::serialize() const
{
    json j;
    j["switch_layers"] = switch_layers;
    j["switch_sheets"] = switch_sheets;
    j["drag_polygon_edges"] = drag_polygon_edges;
    j["drag_to_move"] = drag_to_move;
    return j;
}

void MousePreferences::load_from_json(const json &j)
{
    switch_layers = j.value("switch_layers", true);
    switch_sheets = j.value("switch_sheets", true);
    drag_polygon_edges = j.value("drag_polygon_edges", true);
    drag_to_move = j.value("drag_to_move", true);
}

json UndoRedoPreferences::serialize() const
{
    json j;
    j["show_hints"] = show_hints;
    j["max_depth"] = max_depth;
    j["never_forgets"] = never_forgets;
    return j;
}

void UndoRedoPreferences::load_from_json(const json &j)
{
    show_hints = j.value("show_hints", true);
    max_depth = j.value("max_depth", 50);
    never_forgets = j.value("never_forgets", false);
}

json AppearancePreferences::serialize() const
{
    json j;
    j["dark_theme"] = dark_theme;
    return j;
}

void AppearancePreferences::load_from_json(const json &j)
{
    dark_theme = j.value("dark_theme", false);
}

json ToolBarPreferences::serialize() const
{
    json j;
    j["vertical_layout"] = vertical_layout;
    return j;
}

void ToolBarPreferences::load_from_json(const json &j)
{
    vertical_layout = j.value("vertical_layout", false);
}

json SpacenavPreferences::serialize() const
{
    json j;
    j["threshold"] = prefs.threshold;
    j["pan_scale"] = prefs.pan_scale;
    j["zoom_scale"] = prefs.zoom_scale;
    j["rotation_scale"] = prefs.rotation_scale;
    auto b = json::array();
    for (const auto &it : buttons) {
        b.push_back(action_lut.lookup_reverse(it));
    }
    j["buttons"] = b;
    return j;
}

void SpacenavPreferences::load_from_json(const json &j)
{
    prefs.threshold = j.value("threshold", 10);
    prefs.pan_scale = j.value("pan_scale", .05);
    prefs.zoom_scale = j.value("zoom_scale", .001);
    prefs.rotation_scale = j.value("rotation_scale", .01);
    buttons.clear();
    if (j.count("buttons")) {
        for (const auto &v : j.at("buttons")) {
            buttons.push_back(action_lut.lookup(v.get<std::string>()));
        }
    }
}

static const LutEnumStr<InputDevicesPrefs::Device::Type> type_lut = {
        {"auto", InputDevicesPrefs::Device::Type::AUTO},
        {"touchpad", InputDevicesPrefs::Device::Type::TOUCHPAD},
        {"trackpoint", InputDevicesPrefs::Device::Type::TRACKPOINT},
        {"mouse", InputDevicesPrefs::Device::Type::MOUSE},
};

static InputDevicesPrefs::Device device_from_json(const json &j)
{
    InputDevicesPrefs::Device dev;
    dev.type = type_lut.lookup(j.at("type").get<std::string>());
    return dev;
}

static InputDevicesPrefs::DeviceType device_type_from_json(const json &j)
{
    InputDevicesPrefs::DeviceType dev;
    dev.invert_zoom = j.at("invert_zoom").get<bool>();
    dev.invert_pan = j.at("invert_pan").get<bool>();
    return dev;
}

static json serialize_device(const InputDevicesPrefs::Device &dev)
{
    json j;
    j["type"] = type_lut.lookup_reverse(dev.type);
    return j;
}

static json serialize_device_type(const InputDevicesPrefs::DeviceType &dev)
{
    json j;
    j["invert_zoom"] = dev.invert_zoom;
    j["invert_pan"] = dev.invert_pan;
    return j;
}

json InputDevicesPreferences::serialize() const
{
    json j;
    {
        auto o = json::object();
        for (const auto &[k, v] : prefs.devices) {
            o[k] = serialize_device(v);
        }
        j["devices"] = o;
    }
    {
        auto o = json::object();
        for (const auto &[k, v] : prefs.device_types) {
            o[type_lut.lookup_reverse(k)] = serialize_device_type(v);
        }
        j["device_types"] = o;
    }
    return j;
}

void InputDevicesPreferences::load_from_json(const json &j)
{
    if (j.count("devices")) {
        prefs.devices.clear();
        auto &o = j.at("devices");
        for (const auto &[k, v] : o.items()) {
            prefs.devices.emplace(std::piecewise_construct, std::forward_as_tuple(k),
                                  std::forward_as_tuple(device_from_json(v)));
        }
    }
    if (j.count("device_types")) {
        prefs.device_types.clear();
        auto &o = j.at("device_types");
        for (const auto &[k, v] : o.items()) {
            prefs.device_types.emplace(std::piecewise_construct, std::forward_as_tuple(type_lut.lookup(k)),
                                       std::forward_as_tuple(device_type_from_json(v)));
        }
    }
}

json Preferences::serialize() const
{
    json j;
    j["canvas_layer"] = canvas_layer.serialize();
    j["canvas_non_layer"] = canvas_non_layer.serialize();
    j["schematic"] = schematic.serialize();
    j["key_sequences"] = key_sequences.serialize();
    j["in_tool_key_sequences"] = in_tool_key_sequences.serialize();
    j["board"] = board.serialize();
    j["zoom"] = zoom.serialize();
    j["capture_output"] = capture_output;
    j["stock_info_provider"] = stock_info_provider_lut.lookup_reverse(stock_info_provider);
    j["partinfo"] = partinfo.serialize();
    j["digikey_api"] = digikey_api.serialize();
    j["action_bar"] = action_bar.serialize();
    j["mouse"] = mouse.serialize();
    j["undo_redo"] = undo_redo.serialize();
    j["appearance"] = appearance.serialize();
    j["tool_bar"] = tool_bar.serialize();
    j["spacenav"] = spacenav.serialize();
    j["input_devices"] = input_devices.serialize();
    j["show_pull_request_tools"] = show_pull_request_tools;
    j["hud_debug"] = hud_debug;
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
    if (j.count("in_tool_key_sequences"))
        in_tool_key_sequences.load_from_json(j.at("in_tool_key_sequences"));
    if (j.count("action_bar"))
        action_bar.load_from_json(j.at("action_bar"));
    if (j.count("mouse"))
        mouse.load_from_json(j.at("mouse"));
    key_sequences.append_from_json(json_from_resource("/org/horizon-eda/horizon/imp/keys_default.json"));
    in_tool_key_sequences.append_from_json(
            json_from_resource("/org/horizon-eda/horizon/imp/in_tool_keys_default.json"));
    capture_output = j.value("capture_output", capture_output_default);
    show_pull_request_tools = j.value("show_pull_request_tools", false);
    hud_debug = j.value("hud_debug", false);
    stock_info_provider = stock_info_provider_lut.lookup(j.value("stock_info_provider", "none"));
    if (j.count("partinfo"))
        partinfo.load_from_json(j.at("partinfo"));
    if (j.count("digikey_api"))
        digikey_api.load_from_json(j.at("digikey_api"));
    if (j.count("undo_redo"))
        undo_redo.load_from_json(j.at("undo_redo"));
    if (j.count("appearance"))
        appearance.load_from_json(j.at("appearance"));
    if (j.count("tool_bar"))
        tool_bar.load_from_json(j.at("tool_bar"));
    if (j.count("spacenav"))
        spacenav.load_from_json(j.at("spacenav"));
    if (j.count("input_devices"))
        input_devices.load_from_json(j.at("input_devices"));
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
