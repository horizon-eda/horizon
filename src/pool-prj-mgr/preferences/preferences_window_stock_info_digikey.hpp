#pragma once
#include <gtkmm.h>
#include "util/sqlite.hpp"

namespace horizon {

class DigiKeyApiPreferencesEditor : public Gtk::Box {
public:
    DigiKeyApiPreferencesEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, class Preferences &prefs);
    static DigiKeyApiPreferencesEditor *create(Preferences &prefs);

private:
    Preferences &preferences;
    class DigiKeyApiPreferences &digikey_preferences;
    Gtk::Entry *digikey_client_id_entry = nullptr;
    Gtk::Entry *digikey_client_secret_entry = nullptr;
    Gtk::SpinButton *digikey_max_price_breaks_sp = nullptr;
    Gtk::SpinButton *digikey_cache_days_sp = nullptr;
    Gtk::ComboBoxText *digikey_site_combo = nullptr;
    Gtk::ComboBoxText *digikey_currency_combo = nullptr;
    Gtk::Label *digikey_token_label = nullptr;

    void populate_and_bind_combo(Gtk::ComboBoxText &combo,
                                 const std::vector<std::pair<std::string, std::string>> &items, std::string &value);

    void update_warnings();
    void update_token();
    void clear_cache();
    SQLite::Database db;
};

} // namespace horizon
