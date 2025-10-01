#include "canvas_pdf.hpp"
#include "common/picture.hpp"
#include "export_pdf_util.hpp"
#include <giomm.h>

namespace horizon {

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
    const double sz = to_pt(pic.px_size);
    {
        std::vector<char> picdata;
        picdata.reserve(pic.data->width * pic.data->height * 3);
        for (const auto x : pic.data->data) {
            picdata.push_back((x) & 0xff);
            picdata.push_back((x >> 8) & 0xff);
            picdata.push_back((x >> 16) & 0xff);
        }
        img->SetData(PoDoFo::bufferview{picdata.data(), picdata.size()}, pic.data->width, pic.data->height,
                     PoDoFo::PdfPixelFormat::RGB24, pic.data->width * 3);
    }

    {
        auto img_mask = doc.CreateImage();

        std::vector<char> picdata;
        picdata.reserve(pic.data->width * pic.data->height);
        for (const auto x : pic.data->data) {
            picdata.push_back(((x >> 24) & 0xff) * pic.opacity);
        }

        img_mask->SetData(PoDoFo::bufferview{picdata.data(), picdata.size()}, pic.data->width, pic.data->height,
                          PoDoFo::PdfPixelFormat::Grayscale, pic.data->width);
        img->SetSoftMask(*img_mask);
    }


    painter.DrawImage(*img, to_pt(p.x), to_pt(p.y), sz, sz);
    painter.Restore();
}


PoDoFo::PdfFont &load_font(PoDoFo::PdfDocument &document)
{
    auto bytes = Gio::Resource::lookup_data_global("/org/horizon-eda/horizon/DejaVuSans.ttf");
    gsize size;
    auto data = bytes->get_data(size);
    PoDoFo::bufferview buffer{reinterpret_cast<const char *>(data), size};
    return document.GetFonts().GetOrCreateFontFromBuffer(buffer);
}

} // namespace horizon
