#pragma once
#include <gtkmm.h>
#include "nlohmann/json_fwd.hpp"
#include "common/common.hpp"
#include "common/grid_settings.hpp"

namespace horizon {
using json = nlohmann::json;

class GridController {
public:
    GridController(class MainWindow &win, class CanvasGL &ca, GridSettings *settings);
    void disable();
    void set_spacing_square(int64_t s);
    uint64_t get_spacing_square() const;
    void set_origin(const Coordi &c);
    void apply_settings(const GridSettings::Grid &grid);

private:
    class SpinButtonDim *grid_spin_button_square = nullptr;
    class SpinButtonDim *grid_spin_button_x = nullptr;
    class SpinButtonDim *grid_spin_button_y = nullptr;

    class SpinButtonDim *grid_spin_button_origin_x = nullptr;
    class SpinButtonDim *grid_spin_button_origin_y = nullptr;

    class MainWindow &main_window;
    class CanvasGL &canvas;
    GridSettings *settings;
    void apply();
};
} // namespace horizon
