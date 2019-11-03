#pragma once
#include <gtkmm.h>
#include "core/tool_data_window.hpp"

namespace horizon {

class ToolWindow : public Gtk::Window {
public:
    ToolWindow(Gtk::Window *parent, class ImpInterface *intf);

protected:
    void set_title(const std::string &title);
    void emit_event(ToolDataWindow::Event ev);
    Gtk::HeaderBar *headerbar = nullptr;
    class ImpInterface *interface = nullptr;
};

} // namespace horizon
