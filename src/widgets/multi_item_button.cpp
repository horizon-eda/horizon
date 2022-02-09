#include "multi_item_button.hpp"
#include "multi_item_selector.hpp"

namespace horizon {
MultiItemButton::MultiItemButton() : Gtk::MenuButton()
{
    popover = Gtk::manage(new Gtk::Popover(*this));
    set_popover(*popover);

    label = Gtk::manage(new Gtk::Label);
    label->set_ellipsize(Pango::ELLIPSIZE_END);
    label->show();
    label->set_xalign(0);
    label->set_max_width_chars(0);
    add(*label);
}

void MultiItemButton::construct()
{
    auto &sel = get_selector();
    sel.set_vexpand(true);
    sel.set_size_request(100, 200);
    sel.property_margin() = 4;
    sel.show();
    sel.signal_changed().connect([this] {
        update_label();
        s_signal_changed.emit();
    });
    popover->add(sel);
    update_label();
}

void MultiItemButton::on_toggled()
{
    get_selector().update();
    Gtk::ToggleButton::on_toggled();
}

void MultiItemButton::update()
{
    get_selector().update();
    update_label();
}

void MultiItemButton::set_items(const std::set<UUID> &uus)
{
    get_selector().select_items(uus);
    update_label();
}

std::set<UUID> MultiItemButton::get_items() const
{
    return get_selector().get_selected_items();
}

void MultiItemButton::update_label()
{
    if (get_items().size()) {
        std::string s;
        for (const auto &uu : get_items()) {
            if (s.size())
                s += ", ";
            s += get_item_name(uu);
        }
        label->set_text(s);
    }
    else {
        label->set_text("(None)");
    }
}
} // namespace horizon
