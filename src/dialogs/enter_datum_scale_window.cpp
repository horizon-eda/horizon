#include "enter_datum_scale_window.hpp"
#include "imp/imp_interface.hpp"

namespace horizon {


EnterDatumScaleWindow::EnterDatumScaleWindow(Gtk::Window *parent, class ImpInterface *intf, const std::string &label,
                                             double def)
    : ToolWindow(parent, intf)
{
    set_title("Enter scale");

    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4));
    box->property_margin() = 6;
    auto la = Gtk::manage(new Gtk::Label(label));
    la->set_halign(Gtk::ALIGN_START);
    box->pack_start(*la, false, false, 0);

    sp = Gtk::manage(new Gtk::SpinButton());
    sp->set_range(.1, 10);
    sp->set_increments(.1, .1);
    sp->set_digits(3);
    sp->set_value(def);
    sp->signal_activate().connect([this] { emit_event(ToolDataWindow::Event::OK); });
    sp->signal_value_changed().connect([this] {
        auto data = std::make_unique<ToolDataEnterDatumScaleWindow>();
        data->event = ToolDataWindow::Event::UPDATE;
        data->value = get_value();
        interface->tool_update_data(std::move(data));
    });
    box->pack_start(*sp, false, false, 0);
    box->show_all();
    add(*box);
}

double_t EnterDatumScaleWindow::get_value()
{
    return sp->get_value();
}

} // namespace horizon
