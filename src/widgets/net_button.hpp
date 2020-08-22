#pragma once
#include <gtkmm.h>
#include "util/uuid.hpp"

namespace horizon {

class NetButton : public Gtk::MenuButton {
public:
    NetButton(const class Block &b);
    void set_net(const UUID &uu);
    UUID get_net();
    typedef sigc::signal<void, UUID> type_signal_changed;
    type_signal_changed signal_changed()
    {
        return s_signal_changed;
    }
    void update();

private:
    const Block &block;
    Gtk::Popover *popover;
    class NetSelector *ns;
    void update_label();
    void ns_activated(const UUID &uu);
    UUID net_current;
    void on_toggled() override;

    type_signal_changed s_signal_changed;
};
} // namespace horizon
