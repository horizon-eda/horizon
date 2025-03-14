#include "text_renderer.hpp"
#include "common/text.hpp"
#include "util/geom_util.hpp"
#include "util/str_util.hpp"
#include <sstream>

namespace horizon {


std::pair<Coordf, Coordf> TextRenderer::draw(const Coordf &p, float size, const std::string &rtext, int angle,
                                             TextOrigin origin, ColorP color, int layer, const Options &opts)
{
    angle = wrap_angle(angle);
    bool backwards = (angle > 16384) && (angle <= 49152) && !opts.allow_upside_down;
    float sc = size / 21;
    float yshift = 0;
    switch (origin) {
    case TextOrigin::CENTER:
        yshift = -21 / 2;
        break;
    case TextOrigin::BOTTOM:
        yshift = -21;
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
    float lineskip = size * 1.35 + opts.width;
    if (opts.mirror) {
        lineskip *= -1;
    }
    while (std::getline(ss, line, '\n')) {
        const TextData td{buffer, line, opts.font};

        Placement tf;
        tf.shift.x = p.x;
        tf.shift.y = p.y;

        Placement tr;
        if (opts.flip)
            tr.set_angle(32768 - angle);
        else
            tr.set_angle(angle);
        if ((backwards ^ opts.mirror) ^ opts.flip)
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
        tf.mirror = opts.flip;
        if (opts.center) {
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
            if (opts.draw) {
                draw_line(p0, p1, color, layer, opts.width);
            }
        }
        i_line++;
    }

    Coordf e(opts.width / 2, opts.width / 2);
    return {a - e, b + e};
}


std::pair<Coordf, Coordf> TextRenderer::render(const Text &text, ColorP co, Placement transform, bool rev)
{
    const auto mirror_orig = transform.mirror;
    const auto angle_orig = transform.get_angle();
    transform.accumulate(text.placement);
    const auto text_angle = text.placement.get_angle();
    const auto angle =
            (transform.mirror ^ rev ? 32768 - text_angle : text_angle) + (mirror_orig ^ rev ? -1 : 1) * angle_orig;

    Options opts;
    opts.flip = rev;
    opts.mirror = transform.mirror;
    opts.font = text.font;
    opts.width = text.width;
    opts.allow_upside_down = text.allow_upside_down;

    return draw(transform.shift, text.size, text.overridden ? text.text_override : text.text, angle, text.origin, co,
                text.layer, opts);
}

} // namespace horizon
