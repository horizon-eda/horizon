#pragma once
#include <gtkmm.h>

namespace horizon {
class InputDevicesPreferencesEditor : public Gtk::Box {
public:
    InputDevicesPreferencesEditor(class Preferences &prefs);

private:
    Preferences &preferences;
    std::vector<class PreferencesRowDevice *> rows;
};

} // namespace horizon
