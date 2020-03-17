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

    bool circular = false;
    bool x_first = true;
    bool down = true;
    bool right = true;
    enum class Origin { TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT };
    bool clockwise = false;
    Origin circular_origin = Origin::TOP_LEFT;

    Gtk::Entry *entry_prefix = nullptr;
    Gtk::SpinButton *sp_start = nullptr;
    Gtk::SpinButton *sp_step = nullptr;
    std::set<Gtk::Widget *> widgets_circular;
    std::set<Gtk::Widget *> widgets_axis;


    std::vector<Pad *> pads_sorted;
};
} // namespace horizon
