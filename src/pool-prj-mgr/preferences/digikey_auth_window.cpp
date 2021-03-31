#include "digikey_auth_window.hpp"
#include "util/gtk_util.hpp"
#include "util/util.hpp"
#include "util/sqlite.hpp"
#include "util/uuid.hpp"
#include "util/http_client.hpp"
#include "util/win32_undef.hpp"
#include <iostream>
#include "preferences/preferences.hpp"
#include "preferences/preferences_provider.hpp"
#include "util/stock_info_provider_digikey.hpp"
#include "util/str_util.hpp"

namespace horizon {

DigiKeyAuthWindow *DigiKeyAuthWindow::create()
{
    DigiKeyAuthWindow *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/pool-prj-mgr/preferences/digikey_auth_window.ui");
    x->get_widget_derived("window", w);
    return w;
}

static const std::string redirect_uri = "https://horizon-eda.org/oauth.html";

DigiKeyAuthWindow::DigiKeyAuthWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x)
    : Gtk::Window(cobject), preferences(PreferencesProvider::get_prefs().digikey_api)
{
    GET_WIDGET(code_entry);
    code_entry->signal_changed().connect(sigc::mem_fun(*this, &DigiKeyAuthWindow::update_buttons));
    {
        Gtk::Button *open_browser_button;
        GET_WIDGET(open_browser_button);
        open_browser_button->signal_clicked().connect([this] {
            const std::string uri = "https://api.digikey.com/v1/oauth2/authorize?response_type=code&redirect_uri="
                                    + Glib::uri_escape_string(redirect_uri) + "&client_id=" + preferences.client_id;
            gtk_show_uri_on_window(gobj(), uri.c_str(), GDK_CURRENT_TIME, nullptr);
        });
    }

    GET_WIDGET(cancel_button);
    cancel_button->signal_clicked().connect([this] { close(); });

    GET_WIDGET(login_button);
    login_button->signal_clicked().connect(sigc::mem_fun(*this, &DigiKeyAuthWindow::handle_login));


    {
        Gtk::Label *status_label = nullptr;
        GET_WIDGET(status_label);
        Gtk::Spinner *status_spinner = nullptr;
        GET_WIDGET(status_spinner);
        Gtk::Revealer *status_revealer = nullptr;
        GET_WIDGET(status_revealer);
        status_dispatcher.attach(status_label);
        status_dispatcher.attach(status_spinner);
        status_dispatcher.attach(status_revealer);
        status_dispatcher.attach(this);
        status_dispatcher.signal_notified().connect([this](const StatusDispatcher::Notification &n) {
            is_busy = n.status == StatusDispatcher::Status::BUSY;
            update_buttons();

            if (n.status == StatusDispatcher::Status::DONE) {
                Glib::signal_timeout().connect_once(sigc::mem_fun(*this, &DigiKeyAuthWindow::close), 1000);
            }
        });
    }
    signal_delete_event().connect([this](GdkEventAny *ev) { return is_busy; });

    update_buttons();
    signal_hide().connect([this] { delete this; });
}

void DigiKeyAuthWindow::update_buttons()
{
    cancel_button->set_sensitive(!is_busy);
    login_button->set_sensitive(!is_busy && code_entry->get_text().size());
}

void DigiKeyAuthWindow::handle_login()
{
    client_id = preferences.client_id;
    client_secret = preferences.client_secret;
    code = code_entry->get_text();
    trim(code);
    status_dispatcher.reset("Starting…");

    auto thr = std::thread(&DigiKeyAuthWindow::worker, this);
    thr.detach();
}

void DigiKeyAuthWindow::worker()
{
    status_dispatcher.set_status(StatusDispatcher::Status::BUSY, "Getting token");
    try {
        HTTP::Client client;
        SQLite::Database db(StockInfoProviderDigiKey::get_db_filename(), SQLITE_OPEN_READWRITE);
        const std::string postdata = "client_id=" + client_id + "&client_secret=" + client_secret
                                     + "&grant_type=authorization_code&code=" + code
                                     + "&redirect_uri=" + Glib::uri_escape_string(redirect_uri);
        const auto resp = json::parse(client.post("https://api.digikey.com/v1/oauth2/token", postdata));
        db.execute("BEGIN IMMEDIATE");
        StockInfoProviderDigiKey::update_tokens_from_response(db, resp);
        db.execute("COMMIT");
        status_dispatcher.set_status(StatusDispatcher::Status::DONE, "Done");
    }
    catch (const std::exception &e) {
        status_dispatcher.set_status(StatusDispatcher::Status::ERROR, e.what());
    }
    catch (...) {
        status_dispatcher.set_status(StatusDispatcher::Status::ERROR, "Unknown error");
    }
}

} // namespace horizon
