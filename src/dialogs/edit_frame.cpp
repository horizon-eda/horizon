#include "edit_frame.hpp"
#include "frame/frame.hpp"
#include "widgets/spin_button_dim.hpp"
#include "util/gtk_util.hpp"
#include <iostream>
#include <deque>
#include <algorithm>

namespace horizon {


EditFrameDialog::EditFrameDialog(Gtk::Window *parent, Frame *fr)
    : Gtk::Dialog("Frame properties", *parent,
                  Gtk::DialogFlags::DIALOG_MODAL | Gtk::DialogFlags::DIALOG_USE_HEADER_BAR),
      frame(fr)
{
    add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
    add_button("OK", Gtk::ResponseType::RESPONSE_OK);
    set_default_response(Gtk::ResponseType::RESPONSE_OK);
    // set_default_size(400, 300);

    auto grid = Gtk::manage(new Gtk::Grid());
    grid->set_column_spacing(10);
    grid->set_row_spacing(10);
    grid->set_margin_bottom(20);
    grid->set_margin_top(20);
    grid->set_margin_end(20);
    grid->set_margin_start(20);
    grid->set_halign(Gtk::ALIGN_CENTER);

    int top = 0;

    {
        auto sp_width = Gtk::manage(new SpinButtonDim);
        sp_width->set_range(0, 2000_mm);
        bind_widget(sp_width, frame->width);
        grid_attach_label_and_widget(grid, "Width", sp_width, top);
    }
    {
        auto sp_height = Gtk::manage(new SpinButtonDim);
        sp_height->set_range(0, 2000_mm);
        bind_widget(sp_height, frame->height);
        grid_attach_label_and_widget(grid, "Height", sp_height, top);
    }


    get_content_area()->pack_start(*grid, true, true, 0);

    show_all();
}


} // namespace horizon
