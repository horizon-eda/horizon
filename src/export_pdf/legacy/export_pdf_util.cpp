#include "canvas_pdf.hpp"
#include "common/picture.hpp"
#include "export_pdf_util.hpp"

namespace horizon {

void render_picture(PoDoFo::PdfDocument &doc, PoDoFo::PdfPainter &painter, const Picture &pic, const Placement &tr)
{
    PoDoFo::PdfImage img(&doc);
    Placement pl = tr;
    pl.accumulate(pic.placement);

    {
        std::vector<char> picdata;
        picdata.reserve(pic.data->width * pic.data->height * 3);
        for (const auto x : pic.data->data) {
            picdata.push_back((x)&0xff);
            picdata.push_back((x >> 8) & 0xff);
            picdata.push_back((x >> 16) & 0xff);
        }

        PoDoFo::PdfMemoryInputStream stream(picdata.data(), picdata.size());
        img.SetImageColorSpace(PoDoFo::ePdfColorSpace_DeviceRGB);
        img.SetImageData(pic.data->width, pic.data->height, 8, &stream);
    }

    PoDoFo::PdfImage img_mask(&doc);
    {
        std::vector<char> picdata;
        picdata.reserve(pic.data->width * pic.data->height);
        for (const auto x : pic.data->data) {
            picdata.push_back(((x >> 24) & 0xff) * pic.opacity);
        }

        PoDoFo::PdfMemoryInputStream stream(picdata.data(), picdata.size());
        img_mask.SetImageColorSpace(PoDoFo::ePdfColorSpace_DeviceGray);
        img_mask.SetImageData(pic.data->width, pic.data->height, 8, &stream);
    }

    img.SetImageSoftmask(&img_mask);

    painter.Save();
    const auto fangle = pl.get_angle_rad();

    painter.SetTransformationMatrix(cos(fangle), sin(fangle), -sin(fangle), cos(fangle), to_pt((double)pl.shift.x),
                                    to_pt((double)pl.shift.y));
    const int64_t w = pic.data->width * pic.px_size;
    const int64_t h = pic.data->height * pic.px_size;
    const auto p = Coordd(w, h) / -2;
    const double sz = pic.px_size / (1e3 / CONVERSION_CONSTANT);
    painter.DrawImage(to_pt(p.x), to_pt(p.y), &img, sz, sz);
    painter.Restore();
}
} // namespace horizon
