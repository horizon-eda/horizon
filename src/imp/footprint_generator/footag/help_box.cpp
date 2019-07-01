#include "help_box.hpp"

namespace horizon {
FootagHelpBox::FootagHelpBox(void) : Gtk::ScrolledWindow()
{
    label = Gtk::manage(new Gtk::Label());
    label->set_text("");
    label->show();
    label->set_xalign(0);
    label->set_valign(Gtk::ALIGN_START);
    label->set_line_wrap(true);
    label->set_line_wrap_mode(Pango::WRAP_WORD);
    add(*label);
    set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
}

void FootagHelpBox::showit(const std::string &str)
{
    label->set_markup(str);
}
} // namespace horizon
