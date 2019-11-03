#include "ask_datum_angle.hpp"
#include "widgets/spin_button_angle.hpp"
#include <iostream>

namespace horizon {


AskDatumAngleDialog::AskDatumAngleDialog(Gtk::Window *parent, const std::string &label)
    : Gtk::Dialog("Enter Datum", *parent, Gtk::DialogFlags::DIALOG_MODAL | Gtk::DialogFlags::DIALOG_USE_HEADER_BAR)
{
    add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
    add_button("OK", Gtk::ResponseType::RESPONSE_OK);

    set_default_response(Gtk::ResponseType::RESPONSE_OK);


    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4));
    box->property_margin() = 6;
    auto la = Gtk::manage(new Gtk::Label(label));
    la->set_halign(Gtk::ALIGN_START);
    box->pack_start(*la, false, false, 0);

    sp = Gtk::manage(new SpinButtonAngle());
    sp->signal_activate().connect([this] { response(Gtk::RESPONSE_OK); });
    box->pack_start(*sp, false, false, 0);
    get_content_area()->set_border_width(0);
    get_content_area()->pack_start(*box, true, true, 0);
    show_all();
}
} // namespace horizon
