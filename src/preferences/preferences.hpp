#pragma once
#include "canvas/appearance.hpp"
#include "canvas3d/spacenav_prefs.hpp"
#include "canvas/show_via_span.hpp"
#include "canvas/input_devices_prefs.hpp"
#include "nlohmann/json_fwd.hpp"
#include <sigc++/sigc++.h>
#include <string>
#include "imp/action_catalog.hpp"

namespace horizon {
using json = nlohmann::json;

enum class InToolActionID;

class CanvasPreferences {
public:
    Appearance appearance;
    void load_from_json(const json &j);
    void load_colors_from_json(const json &j);
    json serialize() const;
    json serialize_colors() const;
};

class SchematicPreferences {
public:
    bool show_all_junctions = false;
    bool drag_start_net_line = true;
    bool bend_non_ortho = true;

    void load_from_json(const json &j);
    json serialize() const;
};

class BoardPreferences {
public:
    bool drag_start_track = true;
    bool highlight_on_top = true;
    bool show_text_in_tracks = true;
    bool show_text_in_vias = true;
    ShowViaSpan show_via_span = ShowViaSpan::BLIND_BURIED;
    bool move_using_router = true;

    void load_from_json(const json &j);
    json serialize() const;
};

class KeySequencesPreferences {
public:
    std::map<ActionToolID, std::map<ActionCatalogItem::Availability, std::vector<KeySequence>>> keys;

    void load_from_json(const json &j);
    void append_from_json(const json &j);
    json serialize() const;
};

class InToolKeySequencesPreferences {
public:
    std::map<InToolActionID, std::vector<KeySequence>> keys;

    void load_from_json(const json &j);
    void append_from_json(const json &j);
    json serialize() const;
};

class ZoomPreferences {
public:
    bool smooth_zoom_2d = true;
    bool smooth_zoom_3d = false;
    bool touchpad_pan = false;
    float zoom_factor = 50;
    bool keyboard_zoom_to_cursor = false;

    void load_from_json(const json &j);
    json serialize() const;
};

class UndoRedoPreferences {
public:
    bool show_hints = true;
    unsigned int max_depth = 50;
    bool never_forgets = false;

    void load_from_json(const json &j);
    json serialize() const;
};

class PartInfoPreferences {
public:
    std::string url = "https://dev-partinfo.kitspace.org/graphql";
    std::string preferred_distributor;
    bool ignore_moq_gt_1 = true;
    unsigned int max_price_breaks = 3;
    unsigned int cache_days = 5;

    void load_from_json(const json &j);
    json serialize() const;
};

class DigiKeyApiPreferences {
public:
    std::string client_id;
    std::string client_secret;
    std::string site = "DE";
    std::string currency = "EUR";
    unsigned int max_price_breaks = 3;

    void load_from_json(const json &j);
    json serialize() const;
};

class ActionBarPreferences {
public:
    bool enable = true;
    bool remember = true;
    bool show_in_tool = true;

    void load_from_json(const json &j);
    json serialize() const;
};

class MousePreferences {
public:
    bool switch_layers = true;
    bool switch_sheets = true;
    bool drag_polygon_edges = true;
    bool drag_to_move = true;

    void load_from_json(const json &j);
    json serialize() const;
};

class ToolBarPreferences {
public:
    bool vertical_layout = false;

    void load_from_json(const json &j);
    json serialize() const;
};

class AppearancePreferences {
public:
    bool dark_theme = false;

    void load_from_json(const json &j);
    json serialize() const;
};

class SpacenavPreferences {
public:
    SpacenavPrefs prefs;

    std::vector<ActionID> buttons;

    void load_from_json(const json &j);
    json serialize() const;
};

class InputDevicesPreferences {
public:
    InputDevicesPrefs prefs;

    void load_from_json(const json &j);
    json serialize() const;
};

class Preferences {
public:
    Preferences();
    void set_filename(const std::string &filename);
    void load();
    void load_default();
    void load_from_json(const json &j);
    void save();
    static std::string get_preferences_filename();
    json serialize() const;

    CanvasPreferences canvas_non_layer;
    CanvasPreferences canvas_layer;
    SchematicPreferences schematic;
    BoardPreferences board;
    KeySequencesPreferences key_sequences;
    ZoomPreferences zoom;
    bool capture_output = false;

    enum class StockInfoProviderSel { NONE, PARTINFO, DIGIKEY };
    StockInfoProviderSel stock_info_provider = StockInfoProviderSel::NONE;

    PartInfoPreferences partinfo;
    DigiKeyApiPreferences digikey_api;
    ActionBarPreferences action_bar;
    InToolKeySequencesPreferences in_tool_key_sequences;
    MousePreferences mouse;
    UndoRedoPreferences undo_redo;
    AppearancePreferences appearance;
    ToolBarPreferences tool_bar;
    SpacenavPreferences spacenav;
    InputDevicesPreferences input_devices;

    bool show_pull_request_tools = false;
    bool hud_debug = false;

    typedef sigc::signal<void> type_signal_changed;
    type_signal_changed signal_changed()
    {
        return s_signal_changed;
    }

private:
    std::string filename;
    type_signal_changed s_signal_changed;
};
} // namespace horizon
