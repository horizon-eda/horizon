#include "grid_controller.hpp"
#include "canvas/canvas_gl.hpp"
#include "main_window.hpp"
#include "widgets/spin_button_dim.hpp"
#include "nlohmann/json.hpp"
#include "util/gtk_util.hpp"

namespace horizon {
GridController::GridController(class MainWindow &win, class CanvasGL &ca) : main_window(win), canvas(ca)
{

    auto width_chars = 10;
    grid_spin_button_square = Gtk::manage(new SpinButtonDim());
    grid_spin_button_square->set_width_chars(width_chars);
    grid_spin_button_square->set_range(0.01_mm, 10_mm);
    grid_spin_button_square->set_value(1.25_mm);
    grid_spin_button_square->show();
    main_window.grid_box_square->pack_start(*grid_spin_button_square, true, true, 0);

    grid_spin_button_x = Gtk::manage(new SpinButtonDim());
    grid_spin_button_x->set_width_chars(width_chars);
    grid_spin_button_x->set_range(0.01_mm, 10_mm);
    grid_spin_button_x->set_value(1.25_mm);
    grid_spin_button_x->show();
    main_window.grid_box_rect->pack_start(*grid_spin_button_x, true, true, 0);

    grid_spin_button_y = Gtk::manage(new SpinButtonDim());
    grid_spin_button_y->set_width_chars(width_chars);
    grid_spin_button_y->set_range(0.01_mm, 10_mm);
    grid_spin_button_y->set_value(1.25_mm);
    grid_spin_button_y->show();
    main_window.grid_box_rect->pack_start(*grid_spin_button_y, true, true, 0);

    int top = 1;

    grid_spin_button_origin_x = Gtk::manage(new SpinButtonDim());
    grid_spin_button_origin_x->set_width_chars(width_chars);
    grid_spin_button_origin_x->set_hexpand(true);
    grid_spin_button_origin_x->set_range(-1000_mm, 1000_mm);
    grid_spin_button_origin_x->show();

    grid_spin_button_origin_y = Gtk::manage(new SpinButtonDim());
    grid_spin_button_origin_y->set_width_chars(width_chars);
    grid_spin_button_origin_y->set_range(-1000_mm, 1000_mm);
    grid_spin_button_origin_y->show();

    grid_attach_label_and_widget(main_window.grid_grid, "", grid_spin_button_origin_x, top)
            ->set_markup("X<sub>0</sub>");
    grid_attach_label_and_widget(main_window.grid_grid, "", grid_spin_button_origin_y, top)
            ->set_markup("Y<sub>0</sub>");

    main_window.grid_reset_origin_button->signal_clicked().connect([this] {
        grid_spin_button_origin_x->set_value(0);
        grid_spin_button_origin_y->set_value(0);
    });

    grid_spin_button_square->signal_value_changed().connect(sigc::mem_fun(*this, &GridController::apply));
    grid_spin_button_x->signal_value_changed().connect(sigc::mem_fun(*this, &GridController::apply));
    grid_spin_button_y->signal_value_changed().connect(sigc::mem_fun(*this, &GridController::apply));
    main_window.grid_square_button->signal_toggled().connect(sigc::mem_fun(*this, &GridController::apply));
    grid_spin_button_origin_x->signal_value_changed().connect(sigc::mem_fun(*this, &GridController::apply));
    grid_spin_button_origin_y->signal_value_changed().connect(sigc::mem_fun(*this, &GridController::apply));
    apply();
}

void GridController::disable()
{
    main_window.disable_grid_options();
    grid_spin_button_square->set_sensitive(false);
}

void GridController::set_spacing_square(int64_t s)
{
    main_window.grid_square_button->set_active(true);
    grid_spin_button_square->set_value(s);
}

uint64_t GridController::get_spacing_square() const
{
    return grid_spin_button_square->get_value_as_int();
}

void GridController::apply()
{
    if (main_window.grid_square_button->get_active()) {
        canvas.set_grid_spacing(grid_spin_button_square->get_value_as_int());
    }
    else {
        canvas.set_grid_spacing(grid_spin_button_x->get_value_as_int(), grid_spin_button_y->get_value_as_int());
    }
    canvas.set_grid_origin(
            Coordi(grid_spin_button_origin_x->get_value_as_int(), grid_spin_button_origin_y->get_value_as_int()));
}

enum class GridMode { SQUARE, RECTANGULAR };

static const LutEnumStr<GridMode> mode_lut = {
        {"square", GridMode::SQUARE},
        {"rect", GridMode::RECTANGULAR},
};

void GridController::load_from_json(const json &j)
{

    auto mode = mode_lut.lookup(j.at("mode"));
    if (mode == GridMode::SQUARE) {
        main_window.grid_square_button->set_active(true);
    }
    else {
        main_window.grid_rect_button->set_active(true);
    }
    grid_spin_button_square->set_value(j.at("spacing_square"));
    grid_spin_button_x->set_value(j.at("spacing_x"));
    grid_spin_button_y->set_value(j.at("spacing_y"));
    grid_spin_button_origin_x->set_value(j.at("origin_x"));
    grid_spin_button_origin_y->set_value(j.at("origin_y"));
    apply();
}

json GridController::serialize() const
{
    json j;
    j["mode"] = mode_lut.lookup_reverse(main_window.grid_square_button->get_active() ? GridMode::SQUARE
                                                                                     : GridMode::RECTANGULAR);
    j["spacing_square"] = grid_spin_button_square->get_value_as_int();
    j["spacing_x"] = grid_spin_button_x->get_value_as_int();
    j["spacing_y"] = grid_spin_button_y->get_value_as_int();
    j["origin_x"] = grid_spin_button_origin_x->get_value_as_int();
    j["origin_y"] = grid_spin_button_origin_y->get_value_as_int();
    return j;
}

} // namespace horizon
