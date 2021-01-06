#include "view_angle_window.hpp"
#include "util/gtk_util.hpp"
#include "widgets/spin_button_angle.hpp"

namespace horizon {
ViewAngleWindow::ViewAngleWindow() : Gtk::Window()
{
    set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
    auto headerbar = Gtk::manage(new Gtk::HeaderBar);
    headerbar->set_show_close_button(true);
    headerbar->show();
    headerbar->set_title("Rotate view");

    set_titlebar(*headerbar);
    install_esc_to_close(*this);

    sp = Gtk::manage(new SpinButtonAngle);
    sp->show();
    sp->property_margin() = 10;
    add(*sp);
}

SpinButtonAngle &ViewAngleWindow::get_spinbutton()
{
    return *sp;
}


} // namespace horizon
