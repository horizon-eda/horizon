#include "preferences_window_stock_info_local.hpp"
#include "common/common.hpp"
#include "util/gtk_util.hpp"
#include "util/util.hpp"
#include "preferences/preferences.hpp"
#include <set>
#include <iostream>

namespace horizon {

LocalStockPreferencesEditor::LocalStockPreferencesEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x,
                                                         Preferences &prefs)
    : Gtk::Box(cobject), preferences(prefs), localstock_preferences(preferences.localstock)
{
    GET_WIDGET(localstock_database_entry);
    bind_widget(localstock_database_entry, localstock_preferences.database_path, [this](std::string &v) {
        update_warnings();
        preferences.signal_changed().emit();
    });
    update_warnings();
}

void LocalStockPreferencesEditor::update_warnings()
{

    if (localstock_database_entry->get_text().size() == 0) {
        entry_set_warning(localstock_database_entry, "Required");
    }
    else {
        entry_set_warning(localstock_database_entry, "");
    }
}

LocalStockPreferencesEditor *LocalStockPreferencesEditor::create(Preferences &prefs)
{
    LocalStockPreferencesEditor *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    std::vector<Glib::ustring> widgets = {"localstock_box"};
    x->add_from_resource("/org/horizon-eda/horizon/pool-prj-mgr/preferences/preferences.ui", widgets);
    x->get_widget_derived("localstock_box", w, prefs);
    w->reference();
    return w;
}

} // namespace horizon
