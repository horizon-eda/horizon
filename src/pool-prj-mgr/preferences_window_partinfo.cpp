#include "preferences_window_partinfo.hpp"
#include "common/common.hpp"
#include "util/gtk_util.hpp"
#include "util/util.hpp"
#include "nlohmann/json.hpp"
#include <set>
#include <iostream>


namespace horizon {

#define GET_WIDGET(name)                                                                                               \
    do {                                                                                                               \
        x->get_widget(#name, name);                                                                                    \
    } while (0)

PartinfoPreferencesEditor::PartinfoPreferencesEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x,
                                                     Preferences *prefs, PartInfoPreferences *partinfo_prefs)
    : Gtk::Box(cobject), preferences(prefs), partinfo_preferences(partinfo_prefs)
{
    GET_WIDGET(partinfo_enable_switch);
    GET_WIDGET(partinfo_base_url_entry);
    {
        Gtk::Box *partinfo_preferred_distributor_combo_box;
        GET_WIDGET(partinfo_preferred_distributor_combo_box);
        partinfo_preferred_distributor_combo = Gtk::manage(new Gtk::ComboBoxText(true));
        partinfo_preferred_distributor_combo_box->pack_start(*partinfo_preferred_distributor_combo, true, true, 0);
        partinfo_preferred_distributor_combo->show();
    }

    GET_WIDGET(partinfo_ignore_moq_1_cb);
    GET_WIDGET(partinfo_max_price_breaks_sp);
    GET_WIDGET(partinfo_cache_days_sp);

    partinfo_cache_days_sp->signal_output().connect([this] {
        int v = partinfo_cache_days_sp->get_value_as_int();
        if (v == 1)
            partinfo_cache_days_sp->set_text("one day");
        else
            partinfo_cache_days_sp->set_text(std::to_string(v) + " days");
        return true;
    });
    entry_set_tnum(*partinfo_cache_days_sp);
    partinfo_cache_days_sp->set_value(partinfo_prefs->cache_days);
    partinfo_cache_days_sp->signal_changed().connect([this] {
        partinfo_preferences->cache_days = partinfo_cache_days_sp->get_value_as_int();
        preferences->signal_changed().emit();
    });


    partinfo_preferred_distributor_combo->append("Digikey");
    partinfo_preferred_distributor_combo->append("Farnell");
    partinfo_preferred_distributor_combo->append("Mouser");
    partinfo_preferred_distributor_combo->append("Newark");
    partinfo_preferred_distributor_combo->append("RS");

    bind_widget(partinfo_enable_switch, partinfo_preferences->enable);
    partinfo_enable_switch->property_active().signal_changed().connect(
            [this] { preferences->signal_changed().emit(); });
    bind_widget(partinfo_base_url_entry, partinfo_preferences->url,
                [this](std::string &v) { preferences->signal_changed().emit(); });
    bind_widget(partinfo_ignore_moq_1_cb, partinfo_preferences->ignore_moq_gt_1);
    partinfo_ignore_moq_1_cb->signal_toggled().connect([this] { preferences->signal_changed().emit(); });

    partinfo_preferred_distributor_combo->set_active_text(partinfo_preferences->preferred_distributor);
    partinfo_preferred_distributor_combo->get_entry()->set_text(partinfo_preferences->preferred_distributor);
    partinfo_preferred_distributor_combo->signal_changed().connect([this] {
        partinfo_preferences->preferred_distributor = partinfo_preferred_distributor_combo->get_entry_text();
        update_warnings();
        preferences->signal_changed().emit();
    });

    partinfo_max_price_breaks_sp->set_value(partinfo_prefs->max_price_breaks);
    partinfo_max_price_breaks_sp->signal_changed().connect([this] {
        partinfo_preferences->max_price_breaks = partinfo_max_price_breaks_sp->get_value_as_int();
        preferences->signal_changed().emit();
    });

    update_warnings();
}

void PartinfoPreferencesEditor::update_warnings()
{
    auto txt = partinfo_preferred_distributor_combo->get_entry_text();
    std::string warning;
    if (txt.size() == 0) {
        warning = "Required";
    }
    entry_set_warning(partinfo_preferred_distributor_combo->get_entry(), warning);
}

PartinfoPreferencesEditor *PartinfoPreferencesEditor::create(Preferences *prefs, PartInfoPreferences *partinfo_prefs)
{
    PartinfoPreferencesEditor *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    std::vector<Glib::ustring> widgets = {"partinfo_box", "adjustment5", "adjustment6"};
    x->add_from_resource("/org/horizon-eda/horizon/pool-prj-mgr/preferences.ui", widgets);
    x->get_widget_derived("partinfo_box", w, prefs, partinfo_prefs);
    w->reference();
    return w;
}

} // namespace horizon
