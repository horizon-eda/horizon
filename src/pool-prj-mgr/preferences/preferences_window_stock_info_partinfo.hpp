#pragma once
#include <gtkmm.h>

namespace horizon {

class PartinfoPreferencesEditor : public Gtk::Box {
public:
    PartinfoPreferencesEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, class Preferences &prefs);
    static PartinfoPreferencesEditor *create(Preferences &prefs);

private:
    Preferences &preferences;
    class PartInfoPreferences &partinfo_preferences;
    Gtk::Entry *partinfo_base_url_entry = nullptr;
    Gtk::ComboBoxText *partinfo_preferred_distributor_combo = nullptr;
    Gtk::CheckButton *partinfo_ignore_moq_1_cb = nullptr;
    Gtk::SpinButton *partinfo_max_price_breaks_sp = nullptr;
    Gtk::SpinButton *partinfo_cache_days_sp = nullptr;
    void update_warnings();
};

} // namespace horizon
