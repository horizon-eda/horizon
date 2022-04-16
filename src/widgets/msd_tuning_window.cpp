#include "msd_tuning_window.hpp"
#include "util/msd.hpp"
#include <sstream>
#include <iomanip>
#include "util/util.hpp"

namespace horizon {

MSDTuningWindow::MSDTuningWindow() : Gtk::Window()
{
    set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
    auto header = Gtk::manage(new Gtk::HeaderBar());
    header->set_show_close_button(true);
    header->set_title("Mass spring damper tuning");
    set_titlebar(*header);
    header->show();

    {
        auto bu = Gtk::manage(new Gtk::Button("Reset"));
        bu->signal_clicked().connect(sigc::mem_fun(*this, &MSDTuningWindow::reset));
        header->pack_start(*bu);
        bu->show();
    }

    auto grid = Gtk::manage(new Gtk::Grid);
    grid->property_margin() = 20;
    grid->set_margin_bottom(0);
    grid->set_margin_top(10);
    grid->set_row_spacing(5);
    grid->set_column_spacing(10);

    struct SpInfo {
        Gtk::SpinButton *&sp;
        const std::string name;
        const std::string unit;
    };

    const std::vector<SpInfo> spin_buttons = {
            {sp_mass, "Mass", "kg"},
            {sp_springyness, "Springyness", "N/m"},
            {sp_damping, "Damping", "kg/s"},
            {sp_time, "Time", "s"},
    };

    int left = 0;
    for (auto &it : spin_buttons) {
        it.sp = Gtk::manage(new Gtk::SpinButton());
        it.sp->set_range(0, 1);
        it.sp->set_width_chars(12);
        it.sp->signal_value_changed().connect([this] {
            s_signal_changed.emit();
            area->queue_draw();
        });
        auto sp = it.sp;
        auto unit = it.unit;
        it.sp->signal_output().connect([sp, unit] {
            auto value = sp->get_adjustment()->get_value();

            std::ostringstream stream;
            stream.imbue(get_locale());
            stream << std::fixed << std::setprecision(4) << value << " " << unit;

            sp->set_text(stream.str());
            return true;
        });

        auto la = Gtk::manage(new Gtk::Label(it.name));
        la->get_style_context()->add_class("dim-label");
        la->set_xalign(0);

        grid->attach(*la, left, 0, 1, 1);
        grid->attach(*it.sp, left, 1, 1, 1);
        left++;
    }

    create_layout();
    signal_screen_changed().connect([this](const Glib::RefPtr<Gdk::Screen> &screen) { create_layout(); });

    area = Gtk::manage(new Gtk::DrawingArea);
    area->set_margin_top(5);
    grid->attach(*area, 0, 2, spin_buttons.size(), 1);
    area->set_hexpand(true);
    area->set_vexpand(true);
    area->signal_draw().connect(sigc::mem_fun(*this, &MSDTuningWindow::draw_graph));

    reset();
    sp_mass->set_increments(.0001, .0001);
    sp_springyness->set_increments(.001, .01);
    sp_damping->set_increments(.001, .01);
    sp_time->set_range(.1, 2);
    sp_time->set_increments(.1, .1);
    sp_time->set_value(1);

    add(*grid);
    grid->show_all();

    set_default_size(500, 500);
}

MSD::Params MSDTuningWindow::get_msd_params() const
{
    MSD::Params p;
    p.damping = sp_damping->get_value();
    p.mass = sp_mass->get_value();
    p.springyness = sp_springyness->get_value();
    return p;
}

void MSDTuningWindow::reset()
{
    MSD msd;
    sp_mass->set_value(msd.params.mass);
    sp_springyness->set_value(msd.params.springyness);
    sp_damping->set_value(msd.params.damping);
}

void MSDTuningWindow::create_layout()
{
    layout = create_pango_layout("");
    auto attributes_list = pango_attr_list_new();
    auto attribute_font_features = pango_attr_font_features_new("tnum 1");
    pango_attr_list_insert(attributes_list, attribute_font_features);
    pango_layout_set_attributes(layout->gobj(), attributes_list);
    pango_attr_list_unref(attributes_list);
}

static void draw_arrow(const Cairo::RefPtr<Cairo::Context> &cr, double x0, double y0, double rot)
{
    cr->save();
    cr->translate(x0, y0);
    cr->rotate_degrees(rot);
    cr->move_to(-6, 12);
    cr->line_to(0, 0);
    cr->line_to(+6, 12);
    cr->stroke();
    cr->restore();
}

bool MSDTuningWindow::draw_graph(const Cairo::RefPtr<Cairo::Context> &cr)
{
    auto alloc = area->get_allocation();
    const auto height = alloc.get_height();
    const auto width = alloc.get_width();
    const double ax_border_bottom = 20;
    const double ax_border_left = 50;
    const double ax_margin_bottom = 20;

    Gdk::Cairo::set_source_rgba(cr, get_style_context()->get_color());
    cr->set_line_cap(Cairo::LINE_CAP_ROUND);
    cr->set_line_join(Cairo::LINE_JOIN_ROUND);
    cr->set_line_width(2);

    cr->move_to(ax_border_left, 4);
    cr->line_to(ax_border_left, height - ax_margin_bottom);
    cr->stroke();

    draw_arrow(cr, ax_border_left, 4, 0);

    cr->move_to(0, height - ax_border_bottom - ax_margin_bottom);
    cr->line_to(width - 4, height - ax_border_bottom - ax_margin_bottom);
    cr->stroke();

    draw_arrow(cr, width - 4, height - ax_border_bottom - ax_margin_bottom, 90);


    const double tstop = sp_time->get_value();
    const double y_max = 1.5;
    const double x_max = tstop * 1.1;

    const auto scale_x = (width - ax_border_left) / x_max;
    cr->save();
    cr->set_line_cap(Cairo::LINE_CAP_SQUARE);
    cr->translate(ax_border_left, height - ax_border_bottom - ax_margin_bottom);
    int n = 5;
    for (int i = 1; i <= n; i++) {
        const auto x = (tstop * i) / n;
        const double tick_size = 10;
        cr->move_to(x * scale_x, tick_size);
        cr->line_to(x * scale_x, 0);
        cr->stroke();

        {
            std::ostringstream oss;
            oss.imbue(get_locale());
            oss << std::fixed << std::setprecision(2) << std::setw(4) << x;
            layout->set_text(oss.str());
            auto ext = layout->get_pixel_logical_extents();
            cr->move_to(x * scale_x - ext.get_width() / 2, tick_size);
            layout->show_in_cairo_context(cr);
        }
    }
    cr->restore();

    cr->set_source_rgb(1, 0, 0);
    cr->save();
    cr->translate(ax_border_left, height - ax_border_bottom - ax_margin_bottom);
    cr->move_to(-ax_border_left, 0);
    cr->scale(scale_x, -(height - ax_border_bottom - ax_margin_bottom) / y_max);
    MSD msd;
    msd.params = get_msd_params();
    msd.target = 1;

    cr->line_to(0, 0);
    double tsteady = 0;
    while (msd.get_t() < x_max) {
        if (msd.step(x_max / 1000))
            tsteady = msd.get_t();
        cr->line_to(msd.get_t(), msd.get_s());
    }
    cr->restore();
    cr->stroke();

    cr->save();
    cr->set_line_cap(Cairo::LINE_CAP_SQUARE);
    {
        auto c = get_style_context()->get_color();
        c.set_alpha(.5);
        Gdk::Cairo::set_source_rgba(cr, c);
    }
    cr->translate(ax_border_left, height - ax_border_bottom - ax_margin_bottom);
    cr->move_to(scale_x * tsteady, 0);
    cr->line_to(scale_x * tsteady, -(height - ax_border_bottom - ax_margin_bottom - 4));
    cr->stroke();
    cr->restore();

    return true;
}

} // namespace horizon
