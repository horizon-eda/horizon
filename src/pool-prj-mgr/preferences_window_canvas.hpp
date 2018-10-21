#pragma once
#include <gtkmm.h>

namespace horizon {

class CanvasPreferencesEditor : public Gtk::Box {
public:
    CanvasPreferencesEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, class Preferences *prefs,
                            class CanvasPreferences *canvas_prefs, bool layered);
    static CanvasPreferencesEditor *create(Preferences *prefs, CanvasPreferences *canvas_prefs, bool layered);
    Preferences *preferences;
    CanvasPreferences *canvas_preferences;

private:
    Glib::RefPtr<Gtk::ColorChooser> color_chooser;
    sigc::connection color_chooser_conn;
};


}
