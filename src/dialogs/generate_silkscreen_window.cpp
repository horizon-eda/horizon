#include "generate_silkscreen_window.hpp"
#include "util/gtk_util.hpp"
#include "util/util.hpp"
#include <math.h>

namespace horizon {
GenerateSilkscreenWindow::GenerateSilkscreenWindow(Gtk::Window *parent, ImpInterface *intf, ToolSettings *stg)
    : ToolWindow(parent, intf), settings(dynamic_cast<ToolGenerateSilkscreen::Settings *>(stg))
{
    set_title("Generate silkscreen");

    auto grid = Gtk::manage(new Gtk::Grid());
    grid->set_column_spacing(10);
    grid->set_row_spacing(10);
    grid->set_margin_bottom(20);
    grid->set_margin_top(20);
    grid->set_margin_end(20);
    grid->set_margin_start(20);

    int top = 0;

    sp_silk = Gtk::manage(new SpinButtonDim);
    sp_silk->set_range(0, 5_mm);
    sp_silk->set_value(settings->expand_silk);
    sp_silk->signal_value_changed().connect(sigc::mem_fun(*this, &GenerateSilkscreenWindow::update));
    grid_attach_label_and_widget(grid, "Outline offset", sp_silk, top);

    sp_pad = Gtk::manage(new SpinButtonDim);
    sp_pad->set_range(0, 5_mm);
    sp_pad->set_value(settings->expand_pad);
    sp_pad->signal_value_changed().connect(sigc::mem_fun(*this, &GenerateSilkscreenWindow::update));
    grid_attach_label_and_widget(grid, "Pad offset", sp_pad, top);

    auto defaults_button = Gtk::manage(new Gtk::Button("Restore defaults"));
    defaults_button->show();
    defaults_button->signal_clicked().connect([this] { load_defaults(); });
    grid->attach(*defaults_button, 0, top++, 2, 1);

    grid->set_halign(Gtk::ALIGN_CENTER);

    grid->show_all();
    add(*grid);
}

void GenerateSilkscreenWindow::load_defaults()
{
    auto def = ToolGenerateSilkscreen::Settings();
    if (sp_silk) {
        sp_silk->set_value(def.expand_silk);
    }
    if (sp_pad) {
        sp_pad->set_value(def.expand_pad);
    }
}

void GenerateSilkscreenWindow::update()
{
    if (sp_silk) {
        settings->expand_silk = sp_silk->get_value_as_int();
    }
    if (sp_pad) {
        settings->expand_pad = sp_pad->get_value_as_int();
    }
    emit_event(ToolDataWindow::Event::UPDATE);
}

} // namespace horizon
