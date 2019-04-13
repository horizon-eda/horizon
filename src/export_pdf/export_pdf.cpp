#include "export_pdf.hpp"
#include "canvas/canvas.hpp"
#include <podofo/podofo.h>
#include "util/str_util.hpp"
#include "util/util.hpp"
#include "schematic/schematic.hpp"

namespace horizon {


class CanvasPDF : public Canvas {
public:
    CanvasPDF(PoDoFo::PdfPainterMM *painter, PoDoFo::PdfFont *font, const PDFExportSettings &settings);
    void push() override
    {
    }

    void request_push() override;


private:
    PoDoFo::PdfPainterMM *painter;
    PoDoFo::PdfFont *font;
    const PDFExportSettings &settings;
    const PoDoFo::PdfFontMetrics *metrics;
    void img_line(const Coordi &p0, const Coordi &p1, const uint64_t width, int layer, bool tr) override;
    void img_polygon(const Polygon &poly, bool tr) override;
    void img_draw_text(const Coordf &p, float size, const std::string &rtext, int angle, bool flip, TextOrigin origin,
                       int layer = 10000, uint64_t width = 0, TextData::Font font = TextData::Font::SIMPLEX,
                       bool center = false, bool mirror = false) override;
};

CanvasPDF::CanvasPDF(PoDoFo::PdfPainterMM *p, PoDoFo::PdfFont *f, const PDFExportSettings &s)
    : Canvas::Canvas(), painter(p), font(f), settings(s), metrics(font->GetFontMetrics())
{
    img_mode = true;
}

template <typename T> T to_um(T x)
{
    return x / 1000;
}

template <typename T> T to_pt(T x)
{
    return x * .000002834645669291339;
}

void CanvasPDF::img_line(const Coordi &p0, const Coordi &p1, const uint64_t width, int layer, bool tr)
{
    auto w = std::max(width, settings.min_line_width);
    painter->SetStrokeWidthMM(to_um(w));
    Coordi rp0 = p0;
    Coordi rp1 = p1;
    if (tr) {
        rp0 = transform.transform(p0);
        rp1 = transform.transform(p1);
    }

    painter->DrawLineMM(to_um(rp0.x), to_um(rp0.y), to_um(rp1.x), to_um(rp1.y));
}

void CanvasPDF::img_draw_text(const Coordf &p, float size, const std::string &rtext, int angle, bool flip,
                              TextOrigin origin, int layer, uint64_t width, TextData::Font tfont, bool center,
                              bool mirror)
{
    while (angle < 0)
        angle += 65536;
    angle %= 65536;
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
        double fangle = (tf.get_angle() / 65536.0) * 2 * M_PI;
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

void CanvasPDF::img_polygon(const Polygon &poly, bool tr)
{
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
        painter->Fill();
    }
}


void CanvasPDF::request_push()
{
}

static void cb_nop(std::string, double)
{
}

void export_pdf(const class Schematic &sch, const class PDFExportSettings &settings,
                std::function<void(std::string, double)> cb)
{
    if (!cb)
        cb = &cb_nop;
    cb("Initializing", 0);

    PoDoFo::PdfStreamedDocument document(settings.output_filename.c_str());
    PoDoFo::PdfPainterMM painter;
    auto info = document.GetInfo();
    info->SetCreator("horizon EDA");
    info->SetProducer("horizon EDA");
    if (sch.title_block_values.count("author")) {
        info->SetAuthor(sch.title_block_values.at("author"));
    }
    std::string title = "Schematic";
    if (sch.title_block_values.count("project_title")) {
        title = sch.title_block_values.at("project_title");
    }
    info->SetTitle(title);

    std::vector<const Sheet *> sheets;
    for (const auto &it : sch.sheets) {
        sheets.push_back(&it.second);
    }
    std::sort(sheets.begin(), sheets.end(), [](auto a, auto b) { return a->index < b->index; });


    auto font = document.CreateFont("Helvetica");

    auto outlines = document.GetOutlines();
    auto proot = outlines->CreateRoot(title);

    bool first_outline_item = true;

    CanvasPDF ca(&painter, font, settings);
    for (const auto sheet : sheets) {
        cb("Exporting sheet " + format_m_of_n(sheet->index, sheets.size()), ((double)sheet->index) / sheets.size());

        auto page = document.CreatePage(PoDoFo::PdfRect(0, 0, to_pt(sheet->frame.width), to_pt(sheet->frame.height)));
        painter.SetPage(page);
        painter.SetLineCapStyle(PoDoFo::ePdfLineCapStyle_Round);
        painter.SetFont(font);
        painter.SetColor(0, 0, 0);
        painter.SetTextRenderingMode(PoDoFo::ePdfTextRenderingMode_Invisible);

        ca.update(*sheet);

        painter.FinishPage();


        if (first_outline_item)
            proot->CreateChild(sheet->name, PoDoFo::PdfDestination(page));
        else
            proot->Last()->CreateNext(sheet->name, PoDoFo::PdfDestination(page));
        first_outline_item = false;
    }
    document.Close();
}
} // namespace horizon
