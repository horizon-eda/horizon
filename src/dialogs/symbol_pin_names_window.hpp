#pragma once
#include <gtkmm.h>
#include "util/uuid.hpp"
#include "tool_window.hpp"

namespace horizon {

class SymbolPinNamesWindow : public ToolWindow {
public:
    SymbolPinNamesWindow(Gtk::Window *parent, class ImpInterface *intf, class SchematicSymbol &s);
    void go_to_pin(const UUID &uu);
    UUID get_selected_pin();

private:
    class SchematicSymbol &sym;
    class GatePinEditor *editor = nullptr;
    void handle_import();
};
} // namespace horizon
