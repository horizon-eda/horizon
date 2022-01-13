#include "text_renderer.hpp"
#include "canvas.hpp"

namespace horizon {
CanvasTextRenderer::CanvasTextRenderer(Canvas &canvas) : ca(canvas)
{
}

std::pair<Coordf, Coordf> CanvasTextRenderer::draw(const Coordf &p, float size, const std::string &rtext, int angle,
                                                   TextOrigin origin, ColorP color, int layer, const Options &opts)
{
    if (ca.img_mode)
        ca.img_draw_text(p, size, rtext, angle, opts.flip, origin, layer, opts.width, opts.font, opts.center,
                         opts.mirror);
    ca.begin_group(layer);
    const auto r = TextRenderer::draw(p, size, rtext, angle, origin, color, layer, opts);
    ca.end_group();
    return r;
}

void CanvasTextRenderer::draw_line(const Coordf &a, const Coordf &b, ColorP color, int layer, uint64_t width)
{
    ca.img_line(a.to_coordi(), b.to_coordi(), width, layer, false);
    if (!ca.img_auto_line)
        ca.draw_line(a, b, color, layer, false, width);
}


} // namespace horizon
