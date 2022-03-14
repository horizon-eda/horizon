#include "done_revealer_controller.hpp"

namespace horizon {
void DoneRevealerController::attach(Gtk::Revealer *rev, Gtk::Label *la, Gtk::Button *bu)
{
    revealer = rev;
    label = la;
    button = bu;
    button->set_visible(false);
    button->signal_clicked().connect([this] { revealer->set_reveal_child(false); });
}

void DoneRevealerController::show_success(const std::string &s)
{
    button->set_visible(false);
    label->set_text(s);
    label->set_selectable(false);
    revealer->set_reveal_child(true);
    flash_connection.disconnect();
    flash_connection = Glib::signal_timeout().connect(
            [this] {
                revealer->set_reveal_child(false);
                return false;
            },
            2000);
}

void DoneRevealerController::show_error(const std::string &s)
{
    button->set_visible(true);
    label->set_text(s);
    label->set_selectable(true);
    revealer->set_reveal_child(true);
}

} // namespace horizon
