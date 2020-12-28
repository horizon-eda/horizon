#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "util/uuid.hpp"
#include "tool_window.hpp"

namespace horizon {

class ToolDataEnterDatumScaleWindow : public ToolDataWindow {
public:
    double value = 0;
};

class EnterDatumScaleWindow : public ToolWindow {
public:
    EnterDatumScaleWindow(Gtk::Window *parent, class ImpInterface *intf, const std::string &label, double def);

    double get_value();

private:
    Gtk::SpinButton *sp = nullptr;
};
} // namespace horizon
