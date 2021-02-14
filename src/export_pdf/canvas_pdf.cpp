#include "canvas_pdf.hpp"
#include "common/pdf_export_settings.hpp"
#include "util/str_util.hpp"
#include "util/geom_util.hpp"
#include "common/polygon.hpp"
#include "common/hole.hpp"
#include "canvas/appearance.hpp"
#include "board/plane.hpp"

namespace horizon {


CanvasPDF::CanvasPDF(PoDoFo::PdfPainterMM *p, PoDoFo::PdfFont *f, const PDFExportSettings &s)
    : Canvas::Canvas(), painter(p), font(f), settings(s), metrics(font->GetFontMetrics())
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
    painter->Save();
    auto w = std::max(width, settings.min_line_width);
    painter->SetStrokeWidthMM(to_um(w));
    Coordi rp0 = p0;
    Coordi rp1 = p1;
    if (tr) {
        rp0 = transform.transform(p0);
        rp1 = transform.transform(p1);
    }
    auto color = get_pdf_layer_color(layer);
    painter->SetStrokingColor(color.r, color.g, color.b);
    painter->DrawLineMM(to_um(rp0.x), to_um(rp0.y), to_um(rp1.x), to_um(rp1.y));
    painter->Restore();
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
    font->SetFontSize(to_pt(size) * 1.6);
    while (std::getline(ss, line, '\n')) {
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
        painter->Save();
        Coordi p0(xshift, yshift);
        Coordi pt = tf.transform(p0);

        painter->SetTransformationMatrix(cos(fangle), sin(fangle), -sin(fangle), cos(fangle), to_pt(pt.x), to_pt(pt.y));
        PoDoFo::PdfString pstr(reinterpret_cast<const PoDoFo::pdf_utf8 *>(line.c_str()));
        painter->DrawTextMM(0, to_um(size) / -2, pstr);
        painter->Restore();

        i_line++;
    }
}

void CanvasPDF::img_polygon(const Polygon &ipoly, bool tr)
{
    if (!pdf_layer_visible(ipoly.layer))
        return;
    painter->Save();
    auto color = get_pdf_layer_color(ipoly.layer);
    painter->SetColor(color.r, color.g, color.b);
    painter->SetStrokingColor(color.r, color.g, color.b);
    painter->SetStrokeWidthMM(to_um(settings.min_line_width));
    auto poly = ipoly.remove_arcs(64);
    if (poly.usage == nullptr) { // regular patch
        bool first = true;
        for (const auto &it : poly.vertices) {
            Coordi p = it.position;
            if (tr)
                p = transform.transform(p);
            if (first)
                painter->MoveToMM(to_um(p.x), to_um(p.y));
            else
                painter->LineToMM(to_um(p.x), to_um(p.y));
            first = false;
        }
        painter->ClosePath();
        if (fill)
            painter->Fill();
        else
            painter->Stroke();
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
                        painter->MoveToMM(to_um(p.x), to_um(p.y));
                    else
                        painter->LineToMM(to_um(p.x), to_um(p.y));
                    first = false;
                }
            }
            painter->ClosePath();
        }
        if (fill)
            painter->Fill(true);
        else
            painter->Stroke();
    }
    painter->Restore();
}

void CanvasPDF::img_hole(const Hole &hole)
{
    if (!pdf_layer_visible(PDFExportSettings::HOLES_LAYER))
        return;
    painter->Save();

    auto color = get_pdf_layer_color(PDFExportSettings::HOLES_LAYER);
    painter->SetColor(color.r, color.g, color.b);
    painter->SetStrokingColor(color.r, color.g, color.b);
    painter->SetStrokeWidthMM(to_um(settings.min_line_width));

    auto hole2 = hole;
    if (settings.set_holes_size) {
        hole2.diameter = settings.holes_diameter;
    }
    auto poly = hole2.to_polygon().remove_arcs(64);
    bool first = true;
    for (const auto &it : poly.vertices) {
        Coordi p = it.position;
        p = transform.transform(p);
        if (first)
            painter->MoveToMM(to_um(p.x), to_um(p.y));
        else
            painter->LineToMM(to_um(p.x), to_um(p.y));
        first = false;
    }
    painter->ClosePath();
    if (fill)
        painter->Fill(true);
    else
        painter->Stroke();
    painter->Restore();
}

void CanvasPDF::request_push()
{
}
} // namespace horizon
