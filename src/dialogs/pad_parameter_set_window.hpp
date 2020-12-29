#pragma once
#include <gtkmm.h>
#include <set>
#include "tool_window.hpp"

namespace horizon {

enum class ParameterID;

class PadParameterSetWindow : public ToolWindow {
public:
    PadParameterSetWindow(Gtk::Window *parent, class ImpInterface *intf, std::set<class Pad *> &pads, class IPool &p,
                          class Package &pkg);
    bool go_to_pad(const class UUID &uu);

private:
    void load_pad();

    class IPool &pool;
    Package &pkg;
    std::set<class Pad *> pads;
    class Pad *pad_current = nullptr;
    class ParameterSetEditor *editor = nullptr;
    class PoolBrowserButton *padstack_button = nullptr;
    Gtk::ComboBoxText *combo = nullptr;
    Gtk::Box *box = nullptr;
    Gtk::Box *box2 = nullptr;
    void apply_all(ParameterID id);
    std::set<ParameterID> params_apply_all;
};
} // namespace horizon
