#include "canvas_pdf.hpp"
#include "common/picture.hpp"
#include "export_pdf_util.hpp"

namespace horizon {

#define CONVERSION_CONSTANT 0.002834645669291339

void render_picture(PoDoFo::PdfDocument &doc, PoDoFo::PdfPainter &painter, const Picture &pic, const Placement &tr)
{
    auto img = doc.CreateImage();
    Placement pl = tr;
    pl.accumulate(pic.placement);

    painter.Save();
    const auto fangle = pl.get_angle_rad();
    painter.GraphicsState.SetCurrentMatrix(PoDoFo::Matrix::FromCoefficients(
            cos(fangle), sin(fangle), -sin(fangle), cos(fangle), to_pt((double)pl.shift.x), to_pt((double)pl.shift.y)));
    const int64_t w = pic.data->width * pic.px_size;
    const int64_t h = pic.data->height * pic.px_size;
    const auto p = Coordd(w, h) / -2;
    const double sz = pic.px_size / (1e3 / CONVERSION_CONSTANT);
    painter.DrawImage(*img, to_pt(p.x), to_pt(p.y), sz, sz);
    painter.Restore();
}
} // namespace horizon
