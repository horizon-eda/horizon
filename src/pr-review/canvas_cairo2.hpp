#pragma once
#include "canvas/canvas.hpp"
#include <cairomm/cairomm.h>

namespace horizon {
class CanvasCairo2 : public Canvas {
public:
    CanvasCairo2();
    void push() override
    {
    }
    void request_push() override
    {
    }
    Cairo::RefPtr<Cairo::Surface> get_surface()
    {
        return surface;
    }
    void load(const class Symbol &sym, const Placement &placement = Placement());
    void load(const class Package &pkg);
    Cairo::RefPtr<Cairo::Surface> get_image_surface(double scale = 1, double grid = -1);

private:
    void img_polygon(const Polygon &poly, bool tr) override;
    void img_line(const Coordi &p0, const Coordi &p1, const uint64_t width, int layer, bool tr = true) override;
    void img_hole(const Hole &hole) override;
    void render_pad_names(const Package &pkg);
    void draw_text_box(const Placement &q, float width, float height, const std::string &s, ColorP color, int layer,
                       uint64_t text_width, TextBoxMode mode);

    bool layer_filter = false;
    int current_layer = 0;
    bool cairo_layer_visible(int l) const;
    bool fill = false;
    double min_line_width = 0.1_mm;

    cairo_surface_t *recording_surface;
    Cairo::RefPtr<Cairo::Surface> surface;
    Cairo::RefPtr<Cairo::Context> cr;
};
} // namespace horizon
