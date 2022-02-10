#pragma once
#include <gtkmm.h>
#include "util/uuid.hpp"
#include <set>
#include "util/changeable.hpp"

namespace horizon {

class MultiItemButton : public Gtk::MenuButton, public Changeable {
public:
    MultiItemButton();
    void set_items(const std::set<UUID> &uus);
    std::set<UUID> get_items() const;
    void update();

protected:
    virtual class MultiItemSelector &get_selector() = 0;
    virtual const MultiItemSelector &get_selector() const = 0;
    virtual std::string get_item_name(const UUID &uu) const = 0;
    virtual std::string get_label_text() const;
    void update_label();

    void construct();

private:
    Gtk::Popover *popover;
    void on_toggled() override;
    Gtk::Label *label = nullptr;
};
} // namespace horizon
