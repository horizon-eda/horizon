#pragma once
#include "canvas/appearance.hpp"
#include "nlohmann/json_fwd.hpp"
#include <sigc++/sigc++.h>
#include <string>
#include "imp/action_catalog.hpp"
#include "imp/action.hpp"
#include "core/tool_id.hpp"

namespace horizon {
using json = nlohmann::json;

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

    void load_from_json(const json &j);
    json serialize() const;
};

class BoardPreferences {
public:
    bool drag_start_track = true;

    void load_from_json(const json &j);
    json serialize() const;
};

class KeySequencesPreferences {
public:
    std::map<std::pair<ActionID, ToolID>, std::map<ActionCatalogItem::Availability, std::vector<KeySequence>>> keys;

    void load_from_json(const json &j);
    void append_from_json(const json &j);
    json serialize() const;
};

class ZoomPreferences {
public:
    bool smooth_zoom_2d = true;
    bool smooth_zoom_3d = false;

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
