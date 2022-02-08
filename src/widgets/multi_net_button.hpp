#pragma once
#include <gtkmm.h>
#include "util/uuid.hpp"
#include <set>
#include "util/changeable.hpp"

namespace horizon {

class MultiNetButton : public Gtk::MenuButton, public Changeable {
public:
    MultiNetButton(const class Block &b);
    void set_nets(const std::set<UUID> &uus);
    std::set<UUID> get_nets() const;
    void update();

private:
    const Block &block;
    Gtk::Popover *popover;
    class MultiNetSelector *ns;
    void update_label();
    void on_toggled() override;
    Gtk::Label *label = nullptr;
};
} // namespace horizon
