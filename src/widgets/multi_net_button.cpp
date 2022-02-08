#include "multi_net_button.hpp"
#include "block/block.hpp"
#include "multi_net_selector.hpp"

namespace horizon {
MultiNetButton::MultiNetButton(const Block &b) : Gtk::MenuButton(), block(b)
{
    popover = Gtk::manage(new Gtk::Popover(*this));
    ns = Gtk::manage(new MultiNetSelector(block));
    ns->set_vexpand(true);
    ns->set_size_request(100, 200);
    ns->property_margin() = 4;
    ns->show();
    ns->signal_changed().connect([this] {
        update_label();
        s_signal_changed.emit();
    });
    label = Gtk::manage(new Gtk::Label);
    label->set_ellipsize(Pango::ELLIPSIZE_END);
    label->show();
    label->set_xalign(0);
    label->set_max_width_chars(0);
    add(*label);

    popover->add(*ns);
    set_popover(*popover);
    update_label();
}

void MultiNetButton::on_toggled()
{
    ns->update();
    Gtk::ToggleButton::on_toggled();
}

void MultiNetButton::update()
{
    ns->update();
    update_label();
}

void MultiNetButton::set_nets(const std::set<UUID> &uus)
{
    ns->select_nets(uus);
    update_label();
}

std::set<UUID> MultiNetButton::get_nets() const
{
    return ns->get_selected_nets();
}

void MultiNetButton::update_label()
{
    if (get_nets().size()) {
        std::string s;
        for (const auto &uu : get_nets()) {
            if (s.size())
                s += ", ";
            s += block.get_net_name(uu);
        }
        label->set_text(s);
    }
    else {
        label->set_text("(None)");
    }
}
} // namespace horizon
