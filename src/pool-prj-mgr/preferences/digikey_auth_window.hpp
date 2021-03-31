#pragma once
#include <gtkmm.h>
#include "util/status_dispatcher.hpp"

namespace horizon {
class DigiKeyAuthWindow : public Gtk::Window {
public:
    static DigiKeyAuthWindow *create();
    DigiKeyAuthWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x);

private:
    const class DigiKeyApiPreferences &preferences;
    Gtk::Entry *code_entry = nullptr;
    Gtk::Button *login_button = nullptr;
    Gtk::Button *cancel_button = nullptr;

    std::string client_id;
    std::string client_secret;
    std::string code;
    void worker();
    void handle_login();
    bool is_busy = false;
    void update_buttons();

    StatusDispatcher status_dispatcher;
};
} // namespace horizon
