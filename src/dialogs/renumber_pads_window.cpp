#include "renumber_pads_window.hpp"
#include "pool/package.hpp"
#include "util/gtk_util.hpp"
#include "util/util.hpp"
#include <math.h>

namespace horizon {
RenumberPadsWindow::RenumberPadsWindow(Gtk::Window *parent, ImpInterface *intf, Package *a_pkg,
                                       const std::set<UUID> &a_pads)
    : ToolWindow(parent, intf), pkg(a_pkg)
{
    set_title("Renumber pads");
    for (const auto &it : a_pads) {
        pads.insert(&pkg->pads.at(it));
    }


    auto grid = Gtk::manage(new Gtk::Grid());
    grid->set_column_spacing(10);
    grid->set_row_spacing(10);
    grid->set_margin_bottom(20);
    grid->set_margin_top(20);
    grid->set_margin_end(20);
    grid->set_margin_start(20);

    int top = 0;

    auto fn_update = [this](auto v) { this->renumber(); };

    const int width_chars = 9;
    {
        auto b = make_boolean_ganged_switch(circular, "Axis", "Circular", fn_update);
        grid_attach_label_and_widget(grid, "Mode", b, top)->set_width_chars(width_chars);
    }
    {
        auto b = make_boolean_ganged_switch(x_first, "X, then Y", "Y, then X", fn_update);
        auto l = grid_attach_label_and_widget(grid, "Axis", b, top);
        l->set_width_chars(width_chars);
        widgets_axis.insert(b);
        widgets_axis.insert(l);
    }
    {
        auto b = make_boolean_ganged_switch(down, "Up", "Down", fn_update);
        auto l = grid_attach_label_and_widget(grid, "Y direction", b, top);
        l->set_width_chars(width_chars);
        widgets_axis.insert(b);
        widgets_axis.insert(l);
    }
    {
        auto b = make_boolean_ganged_switch(right, "Left", "Right", fn_update);
        auto l = grid_attach_label_and_widget(grid, "X direction", b, top);
        l->set_width_chars(width_chars);
        widgets_axis.insert(b);
        widgets_axis.insert(l);
    }
    {
        auto b = make_boolean_ganged_switch(clockwise, "CCW", "CW", fn_update);
        auto l = grid_attach_label_and_widget(grid, "Direction", b, top);
        l->set_width_chars(width_chars);
        widgets_circular.insert(b);
        widgets_circular.insert(l);
    }
    {
        auto b = Gtk::manage(new Gtk::ComboBoxText);
        const std::map<Origin, std::string> m = {
                {Origin::TOP_LEFT, "Top left"},
                {Origin::BOTTOM_LEFT, "Bottom left"},
                {Origin::BOTTOM_RIGHT, "Bottom right"},
                {Origin::TOP_RIGHT, "Top right"},
        };
        bind_widget<Origin>(b, m, circular_origin, [this](auto v) { this->renumber(); });
        auto l = grid_attach_label_and_widget(grid, "Origin", b, top);
        l->set_width_chars(width_chars);
        widgets_circular.insert(b);
        widgets_circular.insert(l);
    }
    {
        entry_prefix = Gtk::manage(new Gtk::Entry);
        entry_prefix->signal_changed().connect(sigc::mem_fun(*this, &RenumberPadsWindow::renumber));
        entry_prefix->set_hexpand(true);
        grid_attach_label_and_widget(grid, "Prefix", entry_prefix, top)->set_width_chars(width_chars);
    }
    {
        sp_start = Gtk::manage(new Gtk::SpinButton);
        sp_start->set_range(1, 1000);
        sp_start->set_increments(1, 1);
        sp_start->set_value(1);
        sp_start->signal_value_changed().connect(sigc::mem_fun(*this, &RenumberPadsWindow::renumber));
        grid_attach_label_and_widget(grid, "Start", sp_start, top)->set_width_chars(width_chars);
    }
    {
        sp_step = Gtk::manage(new Gtk::SpinButton);
        sp_step->set_range(1, 10);
        sp_step->set_increments(1, 1);
        sp_step->set_value(1);
        sp_step->signal_value_changed().connect(sigc::mem_fun(*this, &RenumberPadsWindow::renumber));
        grid_attach_label_and_widget(grid, "Step", sp_step, top)->set_width_chars(width_chars);
    }
    grid->show_all();
    add(*grid);

    renumber();
}

static void sort_pads(std::vector<Pad *> &pads, bool x, bool asc)
{
    std::stable_sort(pads.begin(), pads.end(), [x, asc](const Pad *a, const Pad *b) {
        int64_t va, vb;
        if (x) {
            va = a->placement.shift.x;
            vb = b->placement.shift.x;
        }
        else {
            va = a->placement.shift.y;
            vb = b->placement.shift.y;
        }

        if (asc)
            return va < vb;
        else
            return vb < va;
    });
}

static double get_angle(const Coordi &c)
{
    auto a = atan2(c.y, c.x);
    if (a < 0)
        return a + 2 * M_PI;
    else
        return a;
}

static double add_angle(double a, double b)
{
    auto r = a + b;
    while (r >= 2 * M_PI)
        r -= 2 * M_PI;
    while (r < 0)
        r += 2 * M_PI;
    return r;
}

void RenumberPadsWindow::renumber()
{
    for (auto w : widgets_circular) {
        w->set_visible(circular);
    }

    for (auto w : widgets_axis) {
        w->set_visible(!circular);
    }

    pads_sorted.clear();
    std::copy(pads.begin(), pads.end(), std::back_inserter(pads_sorted));
    if (!circular) {
        if (x_first) {
            sort_pads(pads_sorted, true, right);
            sort_pads(pads_sorted, false, !down);
        }
        else {
            sort_pads(pads_sorted, false, !down);
            sort_pads(pads_sorted, true, right);
        }
    }
    else {
        Coordi a = pads_sorted.front()->placement.shift;
        Coordi b = a;
        for (const auto &it : pads_sorted) {
            a = Coordi::min(a, it->placement.shift);
            b = Coordi::max(b, it->placement.shift);
        }
        const Coordi center = (a + b) / 2;
        const Coordi ar = a - center;
        const Coordi br = b - center;

        double angle_offset = 0;
        switch (circular_origin) {
        case Origin::TOP_LEFT:
            angle_offset = get_angle({ar.x, br.y});
            break;

        case Origin::TOP_RIGHT:
            angle_offset = get_angle({br.x, br.y});
            break;

        case Origin::BOTTOM_LEFT:
            angle_offset = get_angle({ar.x, ar.y});
            break;

        case Origin::BOTTOM_RIGHT:
            angle_offset = get_angle({br.x, ar.y});
            break;
        }

        // compensate for rounding errors
        if (clockwise)
            angle_offset += 1e-3;
        else
            angle_offset -= 1e-3;

        std::sort(pads_sorted.begin(), pads_sorted.end(), [this, center, angle_offset](const Pad *x, const Pad *y) {
            auto alpha_a = add_angle(get_angle(x->placement.shift - center), -angle_offset);
            auto alpha_b = add_angle(get_angle(y->placement.shift - center), -angle_offset);

            if (clockwise)
                return alpha_b < alpha_a;
            else
                return alpha_a < alpha_b;
        });
    }
    int n = sp_start->get_value_as_int();
    for (auto &it : pads_sorted) {
        it->name = entry_prefix->get_text() + std::to_string(n);
        n += sp_step->get_value_as_int();
    }
    emit_event(ToolDataWindow::Event::UPDATE);
}

const std::vector<Pad *> &RenumberPadsWindow::get_pads_sorted()
{
    return pads_sorted;
}


} // namespace horizon
