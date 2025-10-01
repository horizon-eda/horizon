#include "canvas_pdf.hpp"
#include "common/pdf_export_settings.hpp"
#include "util/str_util.hpp"
#include "util/geom_util.hpp"
#include "common/polygon.hpp"
#include "common/hole.hpp"
#include "canvas/appearance.hpp"
#include "board/plane.hpp"
#include <sstream>

namespace horizon {

// one pt is 1/72 inch
static constexpr double nm_to_pt_factor = 72. / (25.4_mm);

double to_pt(double x_nm)
{
    return x_nm * nm_to_pt_factor;
}

static double from_pt(double x_pt)
{
    return x_pt / nm_to_pt_factor;
}

CanvasPDF::CanvasPDF(PoDoFo::PdfPainter &p, PoDoFo::PdfFont &f, const PDFExportSettings &s)
    : Canvas::Canvas(), painter(p), font(f), settings(s), metrics(font.GetMetrics())
{
    img_mode = true;
    Appearance apperarance;
    layer_colors = apperarance.layer_colors;
    path.Reset();
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

    auto w = std::max(width, std::max(settings.min_line_width, (uint64_t).001_mm));
    painter.GraphicsState.SetLineWidth(to_pt(w));
    Coordi rp0 = p0;
    Coordi rp1 = p1;
    if (tr) {
        rp0 = transform.transform(p0);
        rp1 = transform.transform(p1);
    }
    auto color = get_pdf_layer_color(layer);
    painter.GraphicsState.SetStrokeColor(PoDoFo::PdfColor(color.r, color.g, color.b));
    painter.DrawLine(to_pt(rp0.x), to_pt(rp0.y), to_pt(rp1.x), to_pt(rp1.y));
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

    painter.TextState.SetFont(font, to_pt(size) * 1.6);
    while (std::getline(ss, line, '\n')) {
        line = TextData::trim(line);
        const int64_t line_width = from_pt(font.GetStringLength(line.c_str(), painter.TextState));

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

        painter.GraphicsState.SetCurrentMatrix(PoDoFo::Matrix::FromCoefficients(cos(fangle), sin(fangle), -sin(fangle),
                                                                                cos(fangle), to_pt(pt.x), to_pt(pt.y)));
        painter.DrawText(line.c_str(), 0, to_pt(size) / -2);
        painter.Restore();

        i_line++;
    }
}

void CanvasPDF::img_polygon(const Polygon &ipoly, bool tr)
{

    if (!pdf_layer_visible(ipoly.layer))
        return;

    auto color = get_pdf_layer_color(ipoly.layer);
    painter.GraphicsState.SetFillColor(PoDoFo::PdfColor(color.r, color.g, color.b));
    painter.GraphicsState.SetStrokeColor(PoDoFo::PdfColor(color.r, color.g, color.b));
    painter.GraphicsState.SetLineWidth(to_pt(settings.min_line_width));
    if (ipoly.usage == nullptr) { // regular patch
        draw_polygon(ipoly, tr);
        if (fill)
            painter.DrawPath(path, PoDoFo::PdfPathDrawMode::Fill);
        else
            painter.DrawPath(path, PoDoFo::PdfPathDrawMode::Stroke);
    }
    else if (auto plane = dynamic_cast<const Plane *>(ipoly.usage.ptr)) {
        for (const auto &frag : plane->fragments) {
            for (const auto &dpath : frag.paths) {
                bool first = true;
                for (const auto &it : dpath) {
                    Coordi p(it.X, it.Y);
                    if (tr)
                        p = transform.transform(p);
                    if (first)
                        path.MoveTo(to_pt(p.x), to_pt(p.y));
                    else
                        path.AddLineTo(to_pt(p.x), to_pt(p.y));
                    first = false;
                }
                path.Close();
            }
        }
        if (fill)
            painter.DrawPath(path, PoDoFo::PdfPathDrawMode::Fill);
        else
            painter.DrawPath(path, PoDoFo::PdfPathDrawMode::Stroke);
    }
    path.Reset();
}

void CanvasPDF::img_hole(const Hole &hole)
{
    if (!pdf_layer_visible(PDFExportSettings::HOLES_LAYER))
        return;

    auto color = get_pdf_layer_color(PDFExportSettings::HOLES_LAYER);
    painter.GraphicsState.SetFillColor(PoDoFo::PdfColor(color.r, color.g, color.b));
    painter.GraphicsState.SetStrokeColor(PoDoFo::PdfColor(color.r, color.g, color.b));
    painter.GraphicsState.SetLineWidth(to_pt(settings.min_line_width));

    auto hole2 = hole;
    if (settings.set_holes_size) {
        hole2.diameter = settings.holes_diameter;
    }
    draw_polygon(hole2.to_polygon(), true);
    if (fill)
        painter.DrawPath(path, PoDoFo::PdfPathDrawMode::Fill);
    else
        painter.DrawPath(path, PoDoFo::PdfPathDrawMode::Stroke);
    path.Reset();
}

// c is the arc center.
// angles must be in radians, c and r must be in mm.
// See "How to determine the control points of a Bézier curve that approximates a
// small circular arc" by Richard ADeVeneza, Nov 2004
// https://www.tinaja.com/glib/bezcirc2.pdf
static Coordd pdf_arc_segment(PoDoFo::PdfPainterPath &path, const Coordd c, const double r, double a0, double a1)
{
    const auto da = a0 - a1;
    assert(da != 0);
    assert(std::abs(da) <= M_PI / 2 + 1e-6);

    // Shift to bisect at x axis
    const auto theta = (a0 + a1) / 2;
    const auto phi = da / 2;

    // Compute points of unit circle for given delta angle
    const auto p0 = Coordd(cos(phi), sin(phi));
    const auto p1 = Coordd((4 - p0.x) / 3, (1 - p0.x) * (3 - p0.x) / (3 * p0.y));
    const auto p2 = Coordd(p1.x, -p1.y);
    const auto p3 = Coordd(p0.x, -p0.y);

    // Transform points
    const auto c1 = p1.rotate(theta) * r + c;
    const auto c2 = p2.rotate(theta) * r + c;
    const auto c3 = p3.rotate(theta) * r + c;

    path.AddCubicBezierTo(to_pt(c1.x), to_pt(c1.y), to_pt(c2.x), to_pt(c2.y), to_pt(c3.x), to_pt(c3.y));
    return c3; // end point
}

static void pdf_arc(PoDoFo::PdfPainterPath &path, const Coordd start, const Coordd c, const Coordd end, bool cw)
{
    const auto r = (start - c).mag();

    // Get angles relative to the x axis
    double a0 = (start - c).angle();
    double a1 = (end - c).angle();

    // Circle or large arc
    if (cw && a0 <= a1) {
        a0 += 2 * M_PI;
    }
    else if (!cw && a0 >= a1) {
        a0 -= 2 * M_PI;
    }

    const double da = (cw) ? -M_PI / 2 : M_PI / 2;
    if (cw) {
        assert(a0 > a1);
    }
    else {
        assert(a0 < a1);
    }
    double e = a1 - a0;
    while (std::abs(e) > 1e-6) {
        const auto d = (cw) ? std::max(e, da) : std::min(e, da);
        const auto a = a0 + d;
        pdf_arc_segment(path, c, r, a0, a);
        a0 = a;
        e = a1 - a0;
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
            path.MoveTo(to_pt(p.x), to_pt(p.y));
        }
        if (it->type == Polygon::Vertex::Type::LINE) {
            if (!first) {
                path.AddLineTo(to_pt(p.x), to_pt(p.y));
            }
        }
        else if (it->type == Polygon::Vertex::Type::ARC) {
            Coordd end = it_next->position;
            Coordd c = project_onto_perp_bisector(end, it->position, it->arc_center);
            if (!first)
                path.AddLineTo(to_pt(p.x), to_pt(p.y));

            if (tr) {
                c = transform.transform(c);
                end = transform.transform(end);
            }
            pdf_arc(path, p, c, end, it->arc_reverse);
        }
        first = false;
    }

    path.Close();
}

void CanvasPDF::request_push()
{
}
} // namespace horizon
