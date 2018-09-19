#include "canvas.hpp"
#include "util/text_data.hpp"
#include <algorithm>
#include <glibmm.h>

namespace horizon {

std::pair<Coordf, Coordf> Canvas::draw_text0(const Coordf &p, float size, const std::string &rtext, int angle,
                                             bool flip, TextOrigin origin, ColorP color, int layer, uint64_t width,
                                             bool draw, TextData::Font font, bool center)
{
    TextData td(rtext, font);
    float sc = size / 21;
    float yshift = 0;
    switch (origin) {
    case TextOrigin::CENTER:
        yshift = -21 / 2;
        break;
    case TextOrigin::BOTTOM:
        yshift = 21 / 2;
        break;
    default:
        yshift = 0;
    }

    while (angle < 0)
        angle += 65536;
    angle %= 65536;

    bool backwards = (angle > 16384) && (angle <= 49152);
    Placement tf;
    tf.shift.x = p.x;
    tf.shift.y = p.y;
    int xshift = 0;
    if (backwards) {
        tf.set_angle(angle - 32768);
        xshift = -td.xright;
    }
    else {
        tf.set_angle(angle);
    }
    tf.mirror = flip;
    if (center) {
        if (backwards) {
            xshift += td.xright / 2;
        }
        else {
            xshift -= td.xright / 2;
        }
    }

    Coordf a = p;
    Coordf b = p;
    for (const auto &li : td.lines) {
        Coordf o0 = li.first;
        o0.x += xshift;
        o0.y += yshift;
        o0 *= sc;

        Coordf o1 = li.second;
        o1.x += xshift;
        o1.y += yshift;
        o1 *= sc;

        Coordf p0 = tf.transform(o0);
        Coordf p1 = tf.transform(o1);
        a = Coordf::min(a, Coordf::min(p0, p1));
        b = Coordf::max(b, Coordf::max(p0, p1));
        if (draw) {
            img_line(Coordi(p0.x, p0.y), Coordi(p1.x, p1.y), width, layer, false);
            draw_line(p0, p1, color, layer, false, width);
        }
    }

    Coordf e(width / 2, width / 2);
    return {a - e, b + e};
}
} // namespace horizon
