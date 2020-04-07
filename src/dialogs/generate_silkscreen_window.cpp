#include "generate_silkscreen_window.hpp"
//#include "pool/package.hpp"
//#include "widgets/spin_button_dim.hpp"
#include "util/gtk_util.hpp"
#include "util/util.hpp"
#include <math.h>

namespace horizon {
GenerateSilkscreenWindow::GenerateSilkscreenWindow(Gtk::Window *parent, ImpInterface *intf, ToolSettings *stg)
    : ToolWindow(parent, intf)
{
    set_title("Generate silkscreen");
    settings = (ToolGenerateSilkscreen::Settings *)stg;

    auto grid = Gtk::manage(new Gtk::Grid());
    grid->set_column_spacing(10);
    grid->set_row_spacing(10);
    grid->set_margin_bottom(20);
    grid->set_margin_top(20);
    grid->set_margin_end(20);
    grid->set_margin_start(20);

    int top = 0;

    const int width_chars = 9;
    sp_silk = Gtk::manage(new SpinButtonDim);
    sp_silk->set_range(-1_mm, 5_mm);
    bind_widget(sp_silk, settings->expand_silk);
    sp_silk->signal_value_changed().connect(sigc::mem_fun(*this, &GenerateSilkscreenWindow::update));
    grid_attach_label_and_widget(grid, "Outline offset", sp_silk, top)->set_width_chars(width_chars);

    sp_pad = Gtk::manage(new SpinButtonDim);
    sp_pad->set_range(-1_mm, 5_mm);
    bind_widget(sp_pad, settings->expand_pad);
    sp_pad->signal_value_changed().connect(sigc::mem_fun(*this, &GenerateSilkscreenWindow::update));
    grid_attach_label_and_widget(grid, "Pad offset", sp_pad, top)->set_width_chars(width_chars);

    grid->show_all();
    add(*grid);
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
