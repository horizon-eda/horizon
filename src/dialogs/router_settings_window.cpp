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

    auto grid = Gtk::manage(new Gtk::Grid());
    grid->set_column_spacing(10);
    grid->set_row_spacing(10);
    grid->set_margin_bottom(20);
    grid->set_margin_top(20);
    grid->set_margin_end(20);
    grid->set_margin_start(20);

    int top = 0;

    {
        mode_combo = Gtk::manage(new Gtk::ComboBoxText);
        mode_combo->set_halign(Gtk::ALIGN_START);
        auto add_mode = [this](auto mode) {
            mode_combo->append(std::to_string(static_cast<int>(mode)),
                               ToolRouteTrackInteractive::Settings::mode_names.at(mode));
        };
        add_mode(Mode::WALKAROUND);
        add_mode(Mode::PUSH);
        add_mode(Mode::BEND);
        add_mode(Mode::STRAIGHT);
        mode_combo->set_active_id(std::to_string(static_cast<int>(settings.mode)));
        mode_combo->signal_changed().connect([this] {
            auto mode = static_cast<Mode>(std::stoi(mode_combo->get_active_id()));
            settings.mode = mode;
            update_drc();
            emit_event(ToolDataWindow::Event::UPDATE);
        });
        grid_attach_label_and_widget(grid, "Mode", mode_combo, top);
    }
    {
        drc_switch = Gtk::manage(new Gtk::Switch);
        drc_switch->set_halign(Gtk::ALIGN_START);
        update_drc();

        drc_switch->property_active().signal_changed().connect([this] {
            if (drc_switch->get_sensitive()) {
                settings.drc = drc_switch->get_active();
                emit_event(ToolDataWindow::Event::UPDATE);
            }
        });
        grid_attach_label_and_widget(grid, "DRC", drc_switch, top);
    }
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

void RouterSettingsWindow::update_drc()
{
    bool drc_forced = any_of(settings.mode, {Mode::WALKAROUND, Mode::PUSH});
    drc_switch->set_sensitive(!drc_forced);
    if (drc_forced) {
        drc_switch->set_active(true);
    }
    else {
        drc_switch->set_active(settings.drc);
    }
}

void RouterSettingsWindow::set_is_routing(bool is_routing)
{
    mode_combo->set_sensitive(!is_routing);
}

} // namespace horizon
