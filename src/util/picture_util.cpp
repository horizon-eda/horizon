#include "picture_util.hpp"
#include "picture_data.hpp"
#include <gdkmm/pixbuf.h>

namespace horizon {
std::shared_ptr<const PictureData> picture_data_from_file(const std::string &filename)
{
    auto pixbuf = Gdk::Pixbuf::create_from_file(filename);
    return picture_data_from_pixbuf(pixbuf);
}

std::shared_ptr<const class PictureData> picture_data_from_pixbuf(Glib::RefPtr<Gdk::Pixbuf> pixbuf)
{
    std::vector<uint32_t> v;
    auto w = pixbuf->get_width();
    auto h = pixbuf->get_height();
    v.resize(w * h);
    const uint8_t *px = pixbuf->get_pixels();
    const auto nch = pixbuf->get_n_channels();
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            auto p = px + (x * nch);
            if (nch == 3) {
                v[y * w + x] = (p[0]) | (p[1] << 8) | (p[2] << 16) | (0xff << 24);
            }
            else if (nch == 4 && pixbuf->get_has_alpha()) {
                v[y * w + x] = (p[0]) | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
            }
        }
        px += pixbuf->get_rowstride();
    }
    return std::make_shared<PictureData>(UUID::random(), w, h, std::move(v));
}
} // namespace horizon
