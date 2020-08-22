#pragma once
#include <gtkmm.h>
#include <set>

namespace horizon {

class PadParameterSetDialog : public Gtk::Dialog {
public:
    PadParameterSetDialog(Gtk::Window *parent, std::set<class Pad *> &pads, class IPool &p, class Package &pkg);

private:
    class IPool &pool;
    class MyParameterSetEditor *editor = nullptr;
    class PoolBrowserButton *padstack_button = nullptr;
};
} // namespace horizon
