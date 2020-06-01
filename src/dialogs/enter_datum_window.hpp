#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "util/uuid.hpp"
#include "tool_window.hpp"

namespace horizon {

class ToolDataEnterDatumWindow : public ToolDataWindow {
public:
    int64_t value = 0;
};

class EnterDatumWindow : public ToolWindow {
public:
    EnterDatumWindow(Gtk::Window *parent, class ImpInterface *intf, const std::string &label, int64_t def = 0);

    void set_range(int64_t lo, int64_t hi);
    void set_step_size(uint64_t sz);
    int64_t get_value();

private:
    class SpinButtonDim *sp = nullptr;
};
} // namespace horizon
