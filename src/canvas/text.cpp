#include "canvas.hpp"
#include "util/text_data.hpp"
#include "util/str_util.hpp"
#include "util/geom_util.hpp"
#include <algorithm>
#include <glibmm.h>

namespace horizon {

std::pair<Coordf, Coordf> Canvas::draw_text(const Coordf &p, float size, const std::string &rtext, int angle,
                                            TextOrigin origin, ColorP color, int layer,
                                            const TextRenderer::Options &opts)
{
    return text_renderer.draw(p, size, rtext, angle, origin, color, layer, opts);
}
} // namespace horizon
