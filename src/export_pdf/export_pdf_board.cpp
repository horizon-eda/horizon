#include "export_pdf.hpp"
#include "canvas_pdf.hpp"
#include "util/podofo_inc.hpp"
#include "util/util.hpp"
#include "board/board.hpp"
#include "export_pdf_util.hpp"

namespace horizon {

static void cb_nop(std::string, double)
{
}

void export_pdf(const class Board &brd, const class PDFExportSettings &settings,
                std::function<void(std::string, double)> cb)
{
    if (!cb)
        cb = &cb_nop;
    cb("Initializing", 0);

    PoDoFo::PdfStreamedDocument document(settings.output_filename.c_str());
    PoDoFo::PdfPainter painter;
    painter.SetPrecision(9);
    auto info = document.GetInfo();
    info->SetCreator("horizon EDA");
    info->SetProducer("horizon EDA");


    auto font = document.CreateFont("Helvetica");

    PDFExportSettings my_settings(settings);
    my_settings.include_text = false; // need to work out text placement
    CanvasPDF ca(painter, *font, my_settings);

    cb("Exporting Board", 0);
    int64_t border_width = 1_mm;
    auto bbox = brd.get_bbox();
    auto width = bbox.second.x - bbox.first.x + border_width * 2;
    auto height = bbox.second.y - bbox.first.y + border_width * 2;

    auto page = document.CreatePage(PoDoFo::PdfRect(0, 0, to_pt(width), to_pt(height)));
    painter.SetPage(page);
    painter.SetLineCapStyle(PoDoFo::ePdfLineCapStyle_Round);
    painter.SetFont(font);
    painter.SetColor(0, 0, 0);
    painter.SetTextRenderingMode(PoDoFo::ePdfTextRenderingMode_Invisible);
    if (settings.mirror) {
        painter.SetTransformationMatrix(-1, 0, 0, 1, to_pt(bbox.second.x + border_width),
                                        to_pt(-bbox.first.y + border_width));
    }
    else {
        painter.SetTransformationMatrix(1, 0, 0, 1, to_pt(-bbox.first.x + border_width),
                                        to_pt(-bbox.first.y + border_width));
    }
    ca.layer_filter = true;
    ca.use_layer_colors = true;

    std::vector<int> layers_sorted;
    for (const auto &it : settings.layers) {
        if (it.second.enabled) {
            layers_sorted.push_back(it.first);
            ca.set_layer_color(it.first, it.second.color);
        }
    }
    std::sort(layers_sorted.begin(), layers_sorted.end());
    if (settings.reverse_layers)
        std::reverse(layers_sorted.begin(), layers_sorted.end());

    for (const auto &[uu, pic] : brd.pictures) {
        if (!pic.on_top)
            render_picture(document, painter, pic);
    }

    unsigned int i_layer = 0;
    for (int layer : layers_sorted) {
        ca.clear();
        ca.current_layer = layer;
        ca.fill = settings.layers.at(layer).mode == PDFExportSettings::Layer::Mode::FILL;
        cb("Exporting layer " + format_m_of_n(i_layer, layers_sorted.size()), ((double)i_layer) / layers_sorted.size());
        ca.update(brd);
        i_layer++;
    }

    for (const auto &[uu, pic] : brd.pictures) {
        if (pic.on_top)
            render_picture(document, painter, pic);
    }

    painter.FinishPage();

    document.Close();
}
} // namespace horizon
