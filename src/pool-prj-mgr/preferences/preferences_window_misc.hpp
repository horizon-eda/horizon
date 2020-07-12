#pragma once
#include <gtkmm.h>

namespace horizon {
class MiscPreferencesEditor : public Gtk::ScrolledWindow {
public:
    MiscPreferencesEditor(class Preferences &prefs);

private:
    Preferences &preferences;
};

} // namespace horizon
