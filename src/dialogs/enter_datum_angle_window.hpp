#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "util/uuid.hpp"
#include "tool_window.hpp"

namespace horizon {

class ToolDataEnterDatumAngleWindow : public ToolDataWindow {
public:
    uint16_t value = 0;
};

class EnterDatumAngleWindow : public ToolWindow {
public:
    EnterDatumAngleWindow(Gtk::Window *parent, class ImpInterface *intf, const std::string &label, uint16_t def);

    uint16_t get_value();

private:
    class SpinButtonAngle *sp = nullptr;
};
} // namespace horizon
