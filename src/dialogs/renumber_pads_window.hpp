#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "util/uuid.hpp"
#include "tool_window.hpp"

namespace horizon {

class RenumberPadsWindow : public ToolWindow {
public:
    RenumberPadsWindow(Gtk::Window *parent, class ImpInterface *intf, class Package *pkg, const std::set<UUID> &pads);

    const std::vector<class Pad *> &get_pads_sorted();
    void renumber();

private:
    class Package *pkg = nullptr;
    std::set<class Pad *> pads;

    bool x_first = true;
    bool down = true;
    bool right = true;
    Gtk::Entry *entry_prefix = nullptr;
    Gtk::SpinButton *sp_start = nullptr;
    Gtk::SpinButton *sp_step = nullptr;

    std::vector<Pad *> pads_sorted;
};
} // namespace horizon
