#include "renumber_pads_window.hpp"
#include "pool/package.hpp"
#include "util/gtk_util.hpp"

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
    grid->set_halign(Gtk::ALIGN_CENTER);

    int top = 0;

    auto fn_update = [this](auto v) { this->renumber(); };

    {
        auto b = make_boolean_ganged_switch(x_first, "X, then Y", "Y, then X", fn_update);
        grid_attach_label_and_widget(grid, "Axis", b, top);
    }
    {
        auto b = make_boolean_ganged_switch(down, "Up", "Down", fn_update);
        grid_attach_label_and_widget(grid, "Y direction", b, top);
    }
    {
        auto b = make_boolean_ganged_switch(right, "Left", "Right", fn_update);
        grid_attach_label_and_widget(grid, "X direction", b, top);
    }
    {
        entry_prefix = Gtk::manage(new Gtk::Entry);
        entry_prefix->signal_changed().connect(sigc::mem_fun(*this, &RenumberPadsWindow::renumber));
        grid_attach_label_and_widget(grid, "Prefix", entry_prefix, top);
    }
    {
        sp_start = Gtk::manage(new Gtk::SpinButton);
        sp_start->set_range(1, 1000);
        sp_start->set_increments(1, 1);
        sp_start->set_value(1);
        sp_start->signal_value_changed().connect(sigc::mem_fun(*this, &RenumberPadsWindow::renumber));
        grid_attach_label_and_widget(grid, "Start", sp_start, top);
    }
    {
        sp_step = Gtk::manage(new Gtk::SpinButton);
        sp_step->set_range(1, 10);
        sp_step->set_increments(1, 1);
        sp_step->set_value(1);
        sp_step->signal_value_changed().connect(sigc::mem_fun(*this, &RenumberPadsWindow::renumber));
        grid_attach_label_and_widget(grid, "Step", sp_step, top);
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

void RenumberPadsWindow::renumber()
{
    pads_sorted.clear();
    std::copy(pads.begin(), pads.end(), std::back_inserter(pads_sorted));
    if (x_first) {
        sort_pads(pads_sorted, true, right);
        sort_pads(pads_sorted, false, !down);
    }
    else {
        sort_pads(pads_sorted, false, !down);
        sort_pads(pads_sorted, true, right);
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
