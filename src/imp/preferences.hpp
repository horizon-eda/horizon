#pragma once
#include "canvas/grid.hpp"
#include "nlohmann/json_fwd.hpp"
#include <sigc++/sigc++.h>
#include <string>
#include "action_catalog.hpp"
#include "action.hpp"
#include "core/core.hpp"

namespace horizon {
using json = nlohmann::json;

class CanvasPreferences {
public:
    enum class BackgroundColor { BLUE, BLACK };
    BackgroundColor background_color = BackgroundColor::BLUE;

    Grid::Style grid_style = Grid::Style::CROSS;
    float grid_opacity = .5;
    float highlight_dim = .3;
    float highlight_shadow = .3;
    float highlight_lighten = .3;
    unsigned int msaa = 0;

    enum class GridFineModifier { CTRL, ALT };
    GridFineModifier grid_fine_modifier = GridFineModifier::ALT;

    void load_from_json(const json &j);
    json serialize() const;
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
    std::map<std::pair<ActionID, ToolID>, std::map<ActionCatalogItem::Availability, std::vector<KeySequence2>>> keys;

    void load_from_json(const json &j);
    void append_from_json(const json &j);
    json serialize() const;
};

class ImpPreferences : public sigc::trackable {
public:
    ImpPreferences();
    void set_filename(const std::string &filename);
    void load();
    void load_default();
    void save();
    bool lock();
    void unlock();
    static std::string get_preferences_filename();

    CanvasPreferences canvas_non_layer;
    CanvasPreferences canvas_layer;
    SchematicPreferences schematic;
    BoardPreferences board;
    KeySequencesPreferences key_sequences;

    typedef sigc::signal<void> type_signal_changed;
    type_signal_changed signal_changed()
    {
        return s_signal_changed;
    }

private:
    std::string filename;
    type_signal_changed s_signal_changed;
    json serialize() const;
};
} // namespace horizon
