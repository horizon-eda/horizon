#include "edit_copper_thief.hpp"
#include "board/copper_thief.hpp"
#include "widgets/spin_button_dim.hpp"
#include "util/gtk_util.hpp"

namespace horizon {

EditCopperThiefDialog::EditCopperThiefDialog(Gtk::Window *parent, CopperThief &t, bool add_mode)
    : Gtk::Dialog("Copper Thieving", *parent, Gtk::DialogFlags::DIALOG_MODAL | Gtk::DialogFlags::DIALOG_USE_HEADER_BAR),
      thief(t)
{
    add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
    add_button("OK", Gtk::ResponseType::RESPONSE_OK);
    set_default_response(Gtk::ResponseType::RESPONSE_OK);

    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 20));
    box->set_margin_start(20);
    box->set_margin_end(20);
    box->set_margin_top(20);
    box->set_margin_bottom(20);
    box->set_halign(Gtk::ALIGN_CENTER);
    box->set_valign(Gtk::ALIGN_CENTER);

    auto grid = Gtk::manage(new Gtk::Grid);
    grid->set_row_spacing(10);
    grid->set_column_spacing(10);
    int top = 0;
    {
        auto shape_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
        shape_box->get_style_context()->add_class("linked");
        auto b1 = Gtk::manage(new Gtk::RadioButton("Round"));
        b1->set_mode(false);
        shape_box->pack_start(*b1, true, true, 0);

        auto b2 = Gtk::manage(new Gtk::RadioButton("Square"));
        b2->set_mode(false);
        b2->join_group(*b1);
        shape_box->pack_start(*b2, true, true, 0);

        std::map<CopperThiefSettings::Shape, Gtk::RadioButton *> shape_widgets = {
                {CopperThiefSettings::Shape::ROUND, b1},
                {CopperThiefSettings::Shape::SQUARE, b2},
        };
        bind_widget<CopperThiefSettings::Shape>(shape_widgets, thief.settings.shape);

        grid_attach_label_and_widget(grid, "Shape", shape_box, top);
    }
    {
        auto sp = Gtk::manage(new SpinButtonDim());
        sp->set_range(.01_mm, 10_mm);
        bind_widget(sp, thief.settings.size);
        grid_attach_label_and_widget(grid, "Size", sp, top);
    }
    {
        auto sp = Gtk::manage(new SpinButtonDim());
        sp->set_range(.01_mm, 10_mm);
        bind_widget(sp, thief.settings.gap);
        grid_attach_label_and_widget(grid, "Gap", sp, top);
    }
    box->pack_start(*grid, false, false, 0);

    if (!add_mode) {
        auto delete_button = Gtk::manage(new Gtk::Button("Delete Copper Thieving"));
        delete_button->signal_clicked().connect([this] {
            delete_requested = true;
            response(Gtk::ResponseType::RESPONSE_OK);
        });
        box->pack_start(*delete_button, false, false, 0);
    }

    get_content_area()->pack_start(*box, true, true, 0);
    show_all();
}

} // namespace horizon
