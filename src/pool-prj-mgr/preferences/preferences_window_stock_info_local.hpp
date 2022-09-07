#pragma once
#include <gtkmm.h>

namespace horizon {

class LocalStockPreferencesEditor : public Gtk::Box {
public:
    LocalStockPreferencesEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, class Preferences &prefs);
    static LocalStockPreferencesEditor *create(Preferences &prefs);

private:
    Preferences &preferences;
    class LocalStockPreferences &localstock_preferences;
    Gtk::Entry *localstock_database_entry = nullptr;
    void update_warnings();
};

} // namespace horizon
