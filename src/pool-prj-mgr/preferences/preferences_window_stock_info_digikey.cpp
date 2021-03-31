#include "preferences_window_stock_info_digikey.hpp"
#include "common/common.hpp"
#include "util/gtk_util.hpp"
#include "util/util.hpp"
#include "preferences/preferences.hpp"
#include <set>
#include <iostream>
#include "util/stock_info_provider_digikey.hpp"
#include "util/sqlite.hpp"
#include "util/win32_undef.hpp"
#include "digikey_auth_window.hpp"

namespace horizon {

void DigiKeyApiPreferencesEditor::populate_and_bind_combo(Gtk::ComboBoxText &combo,
                                                          const std::vector<std::pair<std::string, std::string>> &items,
                                                          std::string &value)
{
    for (const auto &[code, name] : items)
        combo.append(code, name + " (" + code + ")");
    combo.set_active_id(value);
    combo.signal_changed().connect([this, &combo, &value] {
        value = combo.get_active_id();
        preferences.signal_changed().emit();
    });
}

DigiKeyApiPreferencesEditor::DigiKeyApiPreferencesEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x,
                                                         Preferences &prefs)
    : Gtk::Box(cobject), preferences(prefs), digikey_preferences(preferences.digikey_api),
      db(StockInfoProviderDigiKey::get_db_filename(), SQLITE_OPEN_READWRITE)
{
    GET_WIDGET(digikey_client_id_entry);
    GET_WIDGET(digikey_client_secret_entry);
    GET_WIDGET(digikey_max_price_breaks_sp);
    GET_WIDGET(digikey_cache_days_sp);
    GET_WIDGET(digikey_site_combo);
    GET_WIDGET(digikey_currency_combo);
    GET_WIDGET(digikey_token_label);

    digikey_cache_days_sp->signal_output().connect([this] {
        int v = digikey_cache_days_sp->get_value_as_int();
        if (v == 1)
            digikey_cache_days_sp->set_text("one day");
        else
            digikey_cache_days_sp->set_text(std::to_string(v) + " days");
        return true;
    });
    entry_set_tnum(*digikey_cache_days_sp);
    digikey_cache_days_sp->set_value(digikey_preferences.cache_days);
    digikey_cache_days_sp->signal_changed().connect([this] {
        digikey_preferences.cache_days = digikey_cache_days_sp->get_value_as_int();
        preferences.signal_changed().emit();
    });


    bind_widget(digikey_client_id_entry, digikey_preferences.client_id, [this](std::string &v) {
        update_warnings();
        preferences.signal_changed().emit();
    });
    bind_widget(digikey_client_secret_entry, digikey_preferences.client_secret, [this](std::string &v) {
        update_warnings();
        preferences.signal_changed().emit();
    });


    digikey_max_price_breaks_sp->set_value(digikey_preferences.max_price_breaks);
    digikey_max_price_breaks_sp->signal_changed().connect([this] {
        digikey_preferences.max_price_breaks = digikey_max_price_breaks_sp->get_value_as_int();
        preferences.signal_changed().emit();
    });

    {
        const std::vector<std::pair<std::string, std::string>> sites = {
                {"AU", "Australia"},
                {"AT", "Austria"},
                {"BE", "Belgium"},
                {"BG", "Bulgaria"},
                {"CA", "Canada"},
                {"CN", "China"},
                {"CZ", "Czech Republic"},
                {"DK", "Denmark"},
                {"EE", "Estonia"},
                {"FI", "Finland"},
                {"FR", "France"},
                {"DE", "Germany"},
                {"GR", "Greece"},
                {"HK", "Hong Kong"},
                {"HU", "Hungary"},
                {"IN", "India"},
                {"IE", "Ireland"},
                {"IL", "Israel"},
                {"IT", "Italy"},
                {"JP", "Japan"},
                {"KR", "Korea, Republic of"},
                {"LV", "Latvia"},
                {"LT", "Lithuania"},
                {"LU", "Luxembourg"},
                {"MY", "Malaysia"},
                {"MX", "Mexico"},
                {"NL", "Netherlands"},
                {"NZ", "New Zealand"},
                {"NO", "Norway"},
                {"PH", "Philippines"},
                {"PL", "Poland"},
                {"PT", "Portugal"},
                {"RO", "Romania"},
                {"SG", "Singapore"},
                {"SK", "Slovakia"},
                {"SI", "Slovenia"},
                {"ZA", "South Africa"},
                {"ES", "Spain"},
                {"SE", "Sweden"},
                {"CH", "Switzerland"},
                {"TW", "Taiwan, Province of China"},
                {"TH", "Thailand"},
                {"UK", "United Kingdom"},
                {"US", "United States"},
        };
        populate_and_bind_combo(*digikey_site_combo, sites, digikey_preferences.site);
    }

    {
        const std::vector<std::pair<std::string, std::string>> currencies = {
                {"AUD", "Australian Dollar"},
                {"THB", "Baht"},
                {"CAD", "Canadian Dollar"},
                {"CZK", "Czech Koruna"},
                {"DKK", "Danish Krone"},
                {"EUR", "Euro"},
                {"HUF", "Forint"},
                {"HKD", "Hong Kong Dollar"},
                {"INR", "Indian Rupee"},
                {"MYR", "Malaysian Ringgit"},
                {"ILS", "New Israeli Sheqel"},
                {"RON", "New Romanian Leu"},
                {"TWD", "New Taiwan Dollar"},
                {"NZD", "New Zealand Dollar"},
                {"NOK", "Norwegian Krone"},
                {"PHP", "Philippine Piso"},
                {"GBP", "Pound Sterling"},
                {"ZAR", "Rand"},
                {"SGD", "Singapore Dollar"},
                {"SEK", "Swedish Krona"},
                {"CHF", "Swiss Franc"},
                {"USD", "US Dollar"},
                {"KRW", "Won"},
                {"JPY", "Yen"},
                {"CNY", "Yuan Renminbi"},
                {"PLN", "Zloty"},
        };
        populate_and_bind_combo(*digikey_currency_combo, currencies, digikey_preferences.currency);
    }

    {
        Gtk::Button *digikey_sign_in_button;
        GET_WIDGET(digikey_sign_in_button);
        digikey_sign_in_button->signal_clicked().connect([this] {
            auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));
            auto win = DigiKeyAuthWindow::create();
            win->set_transient_for(*top);
            win->present();
            win->signal_hide().connect(sigc::mem_fun(*this, &DigiKeyApiPreferencesEditor::update_token), false);
        });
    }

    {
        Gtk::Button *digikey_clear_cache_button;
        GET_WIDGET(digikey_clear_cache_button);
        digikey_clear_cache_button->signal_clicked().connect(
                sigc::mem_fun(*this, &DigiKeyApiPreferencesEditor::clear_cache));
    }

    update_warnings();
    update_token();
}

static bool needs_trim(const std::string &s)
{
    return s.size() && (isspace(s.front()) || isspace(s.back()));
}

void DigiKeyApiPreferencesEditor::update_warnings()
{
    for (auto entry : {digikey_client_id_entry, digikey_client_secret_entry}) {
        const std::string txt = entry->get_text();
        if (txt.size() == 0) {
            entry_set_warning(entry, "Required");
        }
        else if (needs_trim(txt)) {
            entry_set_warning(entry, "Has trailing/leading whitespace");
        }
        else {
            entry_set_warning(entry, "");
        }
    }
}


void DigiKeyApiPreferencesEditor::update_token()
{
    SQLite::Query q(db, "SELECT datetime(valid_until, 'localtime') FROM tokens WHERE key = 'refresh'");
    if (q.step()) {
        digikey_token_label->set_text("Token valid until " + q.get<std::string>(0)
                                      + "\nThe token will automatically be refreshed.");
    }
    else {
        digikey_token_label->set_text("No token, log in to obtain one.");
    }
}

void DigiKeyApiPreferencesEditor::clear_cache()
{
    db.execute("DELETE FROM cache");
}


DigiKeyApiPreferencesEditor *DigiKeyApiPreferencesEditor::create(Preferences &prefs)
{
    DigiKeyApiPreferencesEditor *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    std::vector<Glib::ustring> widgets = {"digikey_box", "adjustment10", "adjustment11"};
    x->add_from_resource("/org/horizon-eda/horizon/pool-prj-mgr/preferences/preferences.ui", widgets);
    x->get_widget_derived("digikey_box", w, prefs);
    w->reference();
    return w;
}

} // namespace horizon
