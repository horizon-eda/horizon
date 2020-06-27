#include "canvas.hpp"
#include "util/text_data.hpp"
#include "util/str_util.hpp"
#include <algorithm>
#include <glibmm.h>

namespace horizon {

std::pair<Coordf, Coordf> Canvas::draw_text0(const Coordf &p, float size, const std::string &rtext, int angle,
                                             bool flip, TextOrigin origin, ColorP color, int layer, uint64_t width,
                                             bool draw, TextData::Font font, bool center, bool mirror)
{
    if (img_mode)
        img_draw_text(p, size, rtext, angle, flip, origin, layer, width, font, center, mirror);
    while (angle < 0)
        angle += 65536;
    angle %= 65536;
    bool backwards = (angle > 16384) && (angle <= 49152);
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
    Coordf a = p;
    Coordf b = p;

    std::string text(rtext);
    trim(text);
    std::stringstream ss(text);
    std::string line;
    unsigned int n_lines = std::count(text.begin(), text.end(), '\n');
    unsigned int i_line = 0;
    float lineskip = size * 1.35 + width;
    if (mirror) {
        lineskip *= -1;
    }
    begin_group(layer);
    while (std::getline(ss, line, '\n')) {
        TextData td(line, font);

        Placement tf;
        tf.shift.x = p.x;
        tf.shift.y = p.y;

        Placement tr;
        if (flip)
            tr.set_angle(32768 - angle);
        else
            tr.set_angle(angle);
        if ((backwards ^ mirror) ^ flip)
            tf.shift += tr.transform(Coordi(0, -lineskip * (n_lines - i_line)));
        else
            tf.shift += tr.transform(Coordi(0, -lineskip * i_line));

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
                if (!img_auto_line)
                    draw_line(p0, p1, color, layer, false, width);
            }
        }
        i_line++;
    }
    end_group();

    Coordf e(width / 2, width / 2);
    return {a - e, b + e};
}
} // namespace horizon
