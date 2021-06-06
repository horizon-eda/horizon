#pragma once
#include <gtkmm.h>
#include "nlohmann/json_fwd.hpp"
#include "common/common.hpp"

namespace horizon {
using json = nlohmann::json;

struct GridSettings {
    GridSettings(const json &j);
    GridSettings()
    {
    }
    json serialize() const;
    enum class Mode { SQUARE, RECTANGULAR };
    Mode mode;
    Coordi spacing_rect;
    uint64_t spacing_square;
    Coordi origin;
};

class GridController {
public:
    GridController(class MainWindow &win, class CanvasGL &ca);
    void disable();
    json serialize() const;
    void load_from_json(const json &j);
    void set_spacing_square(int64_t s);
    uint64_t get_spacing_square() const;
    void set_origin(const Coordi &c);
    GridSettings get_settings() const;
    void apply_settings(const GridSettings &settings);

private:
    class SpinButtonDim *grid_spin_button_square = nullptr;
    class SpinButtonDim *grid_spin_button_x = nullptr;
    class SpinButtonDim *grid_spin_button_y = nullptr;

    class SpinButtonDim *grid_spin_button_origin_x = nullptr;
    class SpinButtonDim *grid_spin_button_origin_y = nullptr;

    class MainWindow &main_window;
    class CanvasGL &canvas;
    void apply();
};
} // namespace horizon
