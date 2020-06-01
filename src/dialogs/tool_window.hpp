#pragma once
#include <gtkmm.h>
#include "core/tool_data_window.hpp"

namespace horizon {

class ToolWindow : public Gtk::Window {
public:
    ToolWindow(Gtk::Window *parent, class ImpInterface *intf);
    void set_use_ok(bool okay);

protected:
    Gtk::Button *ok_button = nullptr;
    Gtk::Button *cancel_button = nullptr;
    void set_title(const std::string &title);
    void emit_event(ToolDataWindow::Event ev);
    Gtk::HeaderBar *headerbar = nullptr;
    class ImpInterface *interface = nullptr;
};

} // namespace horizon
