#pragma once
#include <gtkmm.h>

namespace horizon {
class SpaceNavPreferencesEditor : public Gtk::Box {
public:
    SpaceNavPreferencesEditor(class Preferences &prefs);

private:
    Preferences &preferences;

    Gtk::ListBox *buttons_listbox = nullptr;
};

} // namespace horizon
