#include "color_box.hpp"

namespace horizon {
ColorBox::ColorBox()
{
    signal_draw().connect([this](const Cairo::RefPtr<Cairo::Context> &cr) -> bool {
        cr->set_source_rgb(color.r, color.g, color.b);
        cr->paint();
        return true;
    });
}

void ColorBox::set_color(const Color &c)
{
    color = c;
    queue_draw();
}
} // namespace horizon
