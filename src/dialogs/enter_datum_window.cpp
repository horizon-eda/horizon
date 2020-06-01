#include "enter_datum_window.hpp"
#include "widgets/spin_button_dim.hpp"
#include "imp/imp_interface.hpp"

namespace horizon {


EnterDatumWindow::EnterDatumWindow(Gtk::Window *parent, class ImpInterface *intf, const std::string &label, int64_t def)
    : ToolWindow(parent, intf)
{
    set_title("Enter datum");

    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4));
    box->property_margin() = 6;
    auto la = Gtk::manage(new Gtk::Label(label));
    la->set_halign(Gtk::ALIGN_START);
    box->pack_start(*la, false, false, 0);

    sp = Gtk::manage(new SpinButtonDim());
    sp->set_margin_start(8);
    sp->set_range(0, 1e9);
    sp->set_value(def);
    sp->signal_activate().connect([this] { emit_event(ToolDataWindow::Event::OK); });
    sp->signal_value_changed().connect([this] {
        auto data = std::make_unique<ToolDataEnterDatumWindow>();
        data->event = ToolDataWindow::Event::UPDATE;
        data->value = get_value();
        interface->tool_update_data(std::move(data));
    });
    box->pack_start(*sp, false, false, 0);
    box->show_all();
    add(*box);
}

void EnterDatumWindow::set_range(int64_t lo, int64_t hi)
{
    sp->set_range(lo, hi);
}

void EnterDatumWindow::set_step_size(uint64_t sz)
{
    sp->set_increments(sz, sz);
}

int64_t EnterDatumWindow::get_value()
{
    return sp->get_value_as_int();
}

} // namespace horizon
