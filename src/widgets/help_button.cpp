#include "help_button.hpp"

namespace horizon {
HelpButton::HelpButton(const std::string &markup)
{
    set_image_from_icon_name("dialog-question-symbolic", Gtk::ICON_SIZE_BUTTON);
    set_relief(Gtk::RELIEF_NONE);
    get_style_context()->add_class("dim-label");
    get_style_context()->add_class("help-button");
    set_valign(Gtk::ALIGN_CENTER);
    auto popover = Gtk::manage(new Gtk::Popover);
    set_popover(*popover);
    auto la = Gtk::manage(new Gtk::Label);
    la->set_markup(markup);
    la->show();
    la->property_margin() = 10;
    la->set_line_wrap(true);
    la->set_max_width_chars(40);
    popover->add(*la);
}

void HelpButton::pack_into(Gtk::Box &box, const std::string &markup)
{
    auto button = Gtk::manage(new HelpButton(markup));
    button->show();
    box.pack_start(*button, false, false, 0);
}

void HelpButton::pack_into(const Glib::RefPtr<Gtk::Builder> &x, const char *widget, const std::string &markup)
{
    Gtk::Box *box;
    x->get_widget(widget, box);
    HelpButton::pack_into(*box, markup);
}

} // namespace horizon
