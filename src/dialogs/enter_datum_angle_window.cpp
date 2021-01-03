#include "enter_datum_angle_window.hpp"
#include "widgets/spin_button_angle.hpp"
#include "imp/imp_interface.hpp"
#include "util/gtk_util.hpp"

namespace horizon {


EnterDatumAngleWindow::EnterDatumAngleWindow(Gtk::Window *parent, class ImpInterface *intf, const std::string &label,
                                             uint16_t def)
    : ToolWindow(parent, intf)
{
    set_title("Enter angle");

    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4));
    box->property_margin() = 6;
    auto la = Gtk::manage(new Gtk::Label(label));
    la->set_halign(Gtk::ALIGN_START);
    box->pack_start(*la, false, false, 0);

    sp = Gtk::manage(new SpinButtonAngle());
    sp->set_value(def);
    spinbutton_connect_activate(sp, [this] { emit_event(ToolDataWindow::Event::OK); });
    sp->signal_value_changed().connect([this] {
        auto data = std::make_unique<ToolDataEnterDatumAngleWindow>();
        data->event = ToolDataWindow::Event::UPDATE;
        data->value = get_value();
        interface->tool_update_data(std::move(data));
    });
    box->pack_start(*sp, false, false, 0);
    box->show_all();
    add(*box);
}

uint16_t EnterDatumAngleWindow::get_value()
{
    return sp->get_value_as_int();
}

} // namespace horizon
