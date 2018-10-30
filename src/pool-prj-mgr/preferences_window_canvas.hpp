#pragma once
#include <gtkmm.h>
#include "preferences/preferences.hpp"

namespace horizon {

class CanvasPreferencesEditor : public Gtk::Box {
public:
    CanvasPreferencesEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, class Preferences *prefs,
                            class CanvasPreferences *canvas_prefs, bool layered);
    static CanvasPreferencesEditor *create(Preferences *prefs, CanvasPreferences *canvas_prefs, bool layered);
    Preferences *preferences;
    CanvasPreferences *canvas_preferences;

private:
    const bool is_layered;
    Gtk::FlowBox *canvas_colors_fb = nullptr;
    Glib::RefPtr<Gtk::ColorChooser> color_chooser;
    sigc::connection color_chooser_conn;
    void handle_export();
    void handle_import();
    void handle_default();
    void load_colors(const json &j);
    void update_color_chooser();
};


} // namespace horizon
