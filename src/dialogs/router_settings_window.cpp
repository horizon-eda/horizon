#include "router_settings_window.hpp"
#include "util/gtk_util.hpp"
#include "util/util.hpp"
#include <math.h>

namespace horizon {
RouterSettingsWindow::RouterSettingsWindow(Gtk::Window *parent, ImpInterface *intf, ToolSettings &stg)
    : ToolWindow(parent, intf), settings(dynamic_cast<ToolRouteTrackInteractive::Settings &>(stg))
{
    set_title("Router settings");
    set_use_ok(false);
    install_esc_to_close(*this);

    auto grid = Gtk::manage(new Gtk::Grid());
    grid->set_column_spacing(10);
    grid->set_row_spacing(10);
    grid->set_margin_bottom(20);
    grid->set_margin_top(20);
    grid->set_margin_end(20);
    grid->set_margin_start(20);

    int top = 0;

    {
        auto sw = Gtk::manage(new Gtk::Switch);
        sw->set_halign(Gtk::ALIGN_START);
        bind_widget(sw, settings.remove_loops);
        sw->property_active().signal_changed().connect([this] { emit_event(ToolDataWindow::Event::UPDATE); });
        grid_attach_label_and_widget(grid, "Remove loops", sw, top);
    }

    {
        auto sc = Gtk::manage(new Gtk::Scale);
        sc->set_range(0, 2);
        sc->set_draw_value(false);
        sc->set_size_request(150, -1);
        sc->set_round_digits(0);
        sc->set_has_origin(false);
        sc->get_adjustment()->set_step_increment(1);
        sc->get_adjustment()->set_page_increment(1);
        sc->add_mark(0, Gtk::POS_BOTTOM, "Low");
        sc->add_mark(1, Gtk::POS_BOTTOM, "Med");
        sc->add_mark(2, Gtk::POS_BOTTOM, "Full");
        sc->set_value(settings.effort);
        sc->signal_value_changed().connect([this, sc] {
            settings.effort = sc->get_value();
            emit_event(ToolDataWindow::Event::UPDATE);
        });
        grid_attach_label_and_widget(grid, "Optimizer effort", sc, top);
    }

    grid->set_halign(Gtk::ALIGN_CENTER);

    grid->show_all();
    add(*grid);
}

} // namespace horizon
