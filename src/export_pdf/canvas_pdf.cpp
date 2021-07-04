#include "canvas_pdf.hpp"
#include "common/pdf_export_settings.hpp"
#include "util/str_util.hpp"
#include "util/geom_util.hpp"
#include "common/polygon.hpp"
#include "common/hole.hpp"
#include "canvas/appearance.hpp"
#include "board/plane.hpp"

namespace horizon {


CanvasPDF::CanvasPDF(PoDoFo::PdfPainterMM &p, PoDoFo::PdfFont &f, const PDFExportSettings &s)
    : Canvas::Canvas(), painter(p), font(f), settings(s), metrics(font.GetFontMetrics())
{
    img_mode = true;
    Appearance apperarance;
    layer_colors = apperarance.layer_colors;
}

bool CanvasPDF::pdf_layer_visible(int l) const
{
    if (layer_filter == false)
        return true;
    else
        return l == current_layer;
}

Color CanvasPDF::get_pdf_layer_color(int layer) const
{
    if (use_layer_colors)
        return get_layer_color(layer);
    else
        return Color(0, 0, 0);
}

void CanvasPDF::img_line(const Coordi &p0, const Coordi &p1, const uint64_t width, int layer, bool tr)
{
    if (!pdf_layer_visible(layer))
        return;
    painter.Save();
    auto w = std::max(width, settings.min_line_width);
    painter.SetStrokeWidthMM(to_um(w));
    Coordi rp0 = p0;
    Coordi rp1 = p1;
    if (tr) {
        rp0 = transform.transform(p0);
        rp1 = transform.transform(p1);
    }
    auto color = get_pdf_layer_color(layer);
    painter.SetStrokingColor(color.r, color.g, color.b);
    painter.DrawLineMM(to_um(rp0.x), to_um(rp0.y), to_um(rp1.x), to_um(rp1.y));
    painter.Restore();
}

void CanvasPDF::img_draw_text(const Coordf &p, float size, const std::string &rtext, int angle, bool flip,
                              TextOrigin origin, int layer, uint64_t width, TextData::Font tfont, bool center,
                              bool mirror)
{
    if (!settings.include_text)
        return;
    if (!pdf_layer_visible(layer))
        return;

    angle = wrap_angle(angle);
    bool backwards = (angle > 16384) && (angle <= 49152);
    float yshift = 0;
    switch (origin) {
    case TextOrigin::CENTER:
        yshift = 0;
        break;
    default:
        yshift = size / 2;
    }

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
    font.SetFontSize(to_pt(size) * 1.6);
    while (std::getline(ss, line, '\n')) {
        line = TextData::trim(line);
        int64_t line_width = metrics->StringWidthMM(line.c_str()) * 1000;

        Placement tf;
        tf.shift.x = p.x;
        tf.shift.y = p.y;

        Placement tr;
        if (flip)
            tr.set_angle(32768 - angle);
        else
            tr.set_angle(angle);
        if (backwards ^ mirror)
            tf.shift += tr.transform(Coordi(0, -lineskip * (n_lines - i_line)));
        else
            tf.shift += tr.transform(Coordi(0, -lineskip * i_line));

        int xshift = 0;
        if (backwards) {
            tf.set_angle(angle - 32768);
            xshift = -line_width;
        }
        else {
            tf.set_angle(angle);
        }
        tf.mirror = flip;
        if (center) {
            if (backwards) {
                xshift += line_width / 2;
            }
            else {
                xshift -= line_width / 2;
            }
        }
        double fangle = tf.get_angle_rad();
        painter.Save();
        Coordi p0(xshift, yshift);
        Coordi pt = tf.transform(p0);

        painter.SetTransformationMatrix(cos(fangle), sin(fangle), -sin(fangle), cos(fangle), to_pt(pt.x), to_pt(pt.y));
        PoDoFo::PdfString pstr(reinterpret_cast<const PoDoFo::pdf_utf8 *>(line.c_str()));
        painter.DrawTextMM(0, to_um(size) / -2, pstr);
        painter.Restore();

        i_line++;
    }
}

void CanvasPDF::img_polygon(const Polygon &ipoly, bool tr)
{
    if (!pdf_layer_visible(ipoly.layer))
        return;
    painter.Save();
    auto color = get_pdf_layer_color(ipoly.layer);
    painter.SetColor(color.r, color.g, color.b);
    painter.SetStrokingColor(color.r, color.g, color.b);
    painter.SetStrokeWidthMM(to_um(settings.min_line_width));
    if (ipoly.usage == nullptr) { // regular patch
        draw_polygon(ipoly, tr);
        if (fill)
            painter.Fill();
        else
            painter.Stroke();
    }
    else if (auto plane = dynamic_cast<const Plane *>(ipoly.usage.ptr)) {
        for (const auto &frag : plane->fragments) {
            for (const auto &path : frag.paths) {
                bool first = true;
                for (const auto &it : path) {
                    Coordi p(it.X, it.Y);
                    if (tr)
                        p = transform.transform(p);
                    if (first)
                        painter.MoveToMM(to_um(p.x), to_um(p.y));
                    else
                        painter.LineToMM(to_um(p.x), to_um(p.y));
                    first = false;
                }
            }
            painter.ClosePath();
        }
        if (fill)
            painter.Fill(true);
        else
            painter.Stroke();
    }
    painter.Restore();
}

void CanvasPDF::img_hole(const Hole &hole)
{
    if (!pdf_layer_visible(PDFExportSettings::HOLES_LAYER))
        return;
    painter.Save();

    auto color = get_pdf_layer_color(PDFExportSettings::HOLES_LAYER);
    painter.SetColor(color.r, color.g, color.b);
    painter.SetStrokingColor(color.r, color.g, color.b);
    painter.SetStrokeWidthMM(to_um(settings.min_line_width));

    auto hole2 = hole;
    if (settings.set_holes_size) {
        hole2.diameter = settings.holes_diameter;
    }
    draw_polygon(hole2.to_polygon(), true);
    if (fill)
        painter.Fill(true);
    else
        painter.Stroke();
    painter.Restore();
}

// Ported from of PoDoFo::PdfPainter::InternalArc. c is the arc center
// angles must be in radians, c and r must be in a PDF "pt" unit.
static void pdf_arc_segment(PoDoFo::PdfPainter &painter, const Coordd c, const double r,
                            const double a0, const double a1, const bool cw)
{
    assert(std::abs(a1 - a0) <= M_PI/2); // A bezier can only approximate an arc of <= 90 deg
    double delta_angle = (M_PI - static_cast<double>(a0 + a1)) / 2.0f;
    if (!cw)
        delta_angle = 2*M_PI - delta_angle;
    const double new_angle = (static_cast<double>(a1 - a0)/ 2.0f);
    std::printf("    segment a0=%f a1=%f da=%f na=%f cw=%i\n",
                a0*180/M_PI, a1*180/M_PI, delta_angle*180/M_PI,
                new_angle*180/M_PI, cw);
    const Coordd r0 = Coordd(cos(new_angle) * r, sin(new_angle) * r);
    const Coordd r2 = Coordd(
        (r * 4.0f - r0.x) / 3.0f,
        ((r * 1.0f - r0.y) * (r0.x - r * 3.0f)) / (3.0f* r0.y)
    );
    const Coordd r1 = Coordd(r2.x, -r2.y);
    const Coordd r3 = Coordd(r0.x, -r0.y);
    const Coordd a = Coordd(sin(delta_angle), cos(delta_angle));
    const Coordd c1 = Coordd(r1.cross(a), r1.dot(a)) + c;
    const Coordd c2 = Coordd(r2.cross(a), r2.dot(a)) + c;
    const Coordd c3 = Coordd(r3.cross(a), r3.dot(a)) + c;
    painter.CubicBezierTo(c1.x, c1.y, c2.x, c2.y, c3.x, c3.y);
}

static void pdf_arc(PoDoFo::PdfPainter &painter, const Coordd start, const Coordd c,
                    const Coordd end, const bool cw)
{
    const auto r = to_pt((start - c).mag());
    const auto ctr = Coordd(to_pt(c.x), to_pt(c.y)) ;
    double a0 = atan2(start.y - c.y, start.x - c.x);
    double a1 = atan2(end.y - c.y, end.x - c.x);
    std::printf("Arc start=%s\n    center=%s\n    end=%s\n    a0=%f a1=%f\n",
                coord_to_string(start).c_str(),
                coord_to_string(c).c_str(),
                coord_to_string(end).c_str(),
                a0*180/M_PI, a1*180/M_PI);

    if (a0 < 0)
        a0 += 2*M_PI;
    if (a1 < 0)
        a1 += 2*M_PI;

    if (cw) {
        if (a0 >= a1) { // Circle or large arc
            a1 += 2*M_PI;
        }
        assert(a0 < a1);
    }
    else {
        if (a0 >= a1) { // Circle or large arc
            a1 += 2*M_PI;
        }
        assert(a1 > a0);
    }
    while (std::abs(a0 - a1) > M_PI/2) {
        if (cw) {
            const auto a = a0 + M_PI/2;
            pdf_arc_segment(painter, ctr, r, a0, a, cw);
            a0 = a;
        } else {
            const auto a = a1 - M_PI/2;
            pdf_arc_segment(painter, ctr, r, a, a1, cw);
            a1 = a;
        }
    }
    if (a0 != a1) {
        pdf_arc_segment(painter, ctr, r, a0, a1, cw);
    }
}



void CanvasPDF::draw_polygon(const Polygon &ipoly, bool tr)
{
    assert(ipoly.usage == nullptr);
    bool first = true;
    for (auto it = ipoly.vertices.cbegin(); it < ipoly.vertices.cend(); it++) {
        Coordd p = it->position;
        if (tr)
            p = transform.transform(p);
        auto it_next = it + 1;
        if (it_next == ipoly.vertices.cend()) {
            it_next = ipoly.vertices.cbegin();
        }
        if (first) {
            painter.MoveToMM(to_um(p.x), to_um(p.y));
        }
        if (it->type == Polygon::Vertex::Type::LINE) {
            if (!first)
                painter.LineToMM(to_um(p.x), to_um(p.y));
        }
        else if (it->type == Polygon::Vertex::Type::ARC) {
            // Finish arc
            Coordd end = it_next->position;
            Coordd c = it->arc_center;

            if (!first)
                painter.LineToMM(to_um(p.x), to_um(p.y));

            if (tr) {
                c = transform.transform(c);
                end = transform.transform(end);
            }
            pdf_arc(painter, p, c, end, it->arc_reverse);
        }
        first = false;
    }

    painter.ClosePath();
}

void CanvasPDF::request_push()
{
}
} // namespace horizon
