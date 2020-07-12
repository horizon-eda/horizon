#pragma once
#include <gtkmm.h>
#include "preferences/preferences.hpp"
#include "nlohmann/json.hpp"

namespace horizon {
using json = nlohmann::json;
class CanvasPreferencesEditor : public Gtk::Box {
public:
    CanvasPreferencesEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, class Preferences &prefs,
                            bool layered);
    static CanvasPreferencesEditor *create(Preferences &prefs, bool layered);

private:
    Preferences &preferences;
    CanvasPreferences &canvas_preferences;
    const bool is_layered;
    json color_presets;
    Gtk::FlowBox *canvas_colors_fb = nullptr;
    Glib::RefPtr<Gtk::ColorChooser> color_chooser;
    sigc::connection color_chooser_conn;
    void handle_export();
    void handle_import();
    void handle_default();
    void handle_load_preset(unsigned int idx);
    void load_colors(const json &j);
    void update_color_chooser();
};


} // namespace horizon
