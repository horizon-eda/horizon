#include "picture_load.hpp"
#include <glibmm/miscutils.h>
#include <glibmm/fileutils.h>
#include <giomm/file.h>
#include <cairomm/cairomm.h>
#include <set>
#include "util/util.hpp"
#include <algorithm>

namespace horizon {

static std::string build_pic_filename(const std::string &dir, const UUID &uu, const std::string &suffix)
{
    return Glib::build_filename(dir, (std::string)uu + "_" + suffix + ".png");
}

void pictures_load(const std::list<std::map<UUID, Picture> *> &pictures, const std::string &dir,
                   const std::string &suffix)
{
    std::map<UUID, std::shared_ptr<const PictureData>> pictures_loaded;
    for (const auto &it_map : pictures) {
        for (auto &it : *it_map) {
            if (pictures_loaded.count(it.second.data_uuid)) {
                it.second.data = pictures_loaded.at(it.second.data_uuid);
            }
            else {
                auto pic_filename = build_pic_filename(dir, it.second.data_uuid, suffix);
                if (Glib::file_test(pic_filename, Glib::FILE_TEST_IS_REGULAR)) {
                    auto surf = Cairo::ImageSurface::create_from_png(pic_filename);
                    if (surf->get_format() != Cairo::FORMAT_ARGB32 && surf->get_format() != Cairo::FORMAT_RGB24) {
                        throw std::runtime_error("pic must be ARGB32/RGB24");
                    }
                    bool has_alpha = surf->get_format() == Cairo::FORMAT_ARGB32;
                    surf->flush();
                    auto w = surf->get_width();
                    auto h = surf->get_height();
                    std::vector<uint32_t> vd;
                    vd.resize(w * h);
                    auto dp = surf->get_data();
                    for (int y = 0; y < h; y++) {
                        auto p = dp;
                        for (int x = 0; x < w; x++) {
                            if (has_alpha)
                                vd[y * w + x] = (p[3] << 24) | (p[0] << 16) | (p[1] << 8) | p[2];
                            else
                                vd[y * w + x] = (0xff << 24) | (p[0] << 16) | (p[1] << 8) | p[2];
                            p += 4;
                        }
                        dp += surf->get_stride();
                    }
                    auto data = std::make_shared<PictureData>(it.second.data_uuid, w, h, std::move(vd));
                    it.second.data = data;
                    pictures_loaded.emplace(it.second.data_uuid, data);
                }
            }
        }
    }
}

void pictures_save(const std::list<const std::map<UUID, Picture> *> &pictures, const std::string &pic_dir,
                   const std::string &suffix)
{
    bool has_pics = std::any_of(pictures.begin(), pictures.end(), [](const auto &x) { return x->size(); });
    if (has_pics && !Glib::file_test(pic_dir, Glib::FILE_TEST_IS_DIR)) {
        Gio::File::create_for_path(pic_dir)->make_directory_with_parents();
    }
    std::set<UUID> pictures_to_delete;
    {
        Glib::Dir dir(pic_dir);
        for (const auto &it : dir) {
            if (endswith(it, "_" + suffix + ".png")) {
                pictures_to_delete.emplace(it.substr(0, 36));
            }
        }
    }
    for (const auto &it_map : pictures) {
        for (const auto &it : *it_map) {
            if (it.second.data) {
                auto data = it.second.data;
                pictures_to_delete.erase(it.second.data->uuid);
                auto pic_filename = build_pic_filename(pic_dir, it.second.data_uuid, suffix);
                if (!Glib::file_test(pic_filename, Glib::FILE_TEST_IS_REGULAR)) {
                    auto surf = Cairo::ImageSurface::create(Cairo::Format::FORMAT_ARGB32, data->width, data->height);
                    auto dp = surf->get_data();
                    surf->flush();
                    for (unsigned int y = 0; y < data->height; y++) {
                        auto p = dp;
                        for (unsigned int x = 0; x < data->width; x++) {
                            auto v = data->data[y * data->width + x];
                            p[3] = (v >> 24) & 0xff;
                            p[0] = (v >> 16) & 0xff;
                            p[1] = (v >> 8) & 0xff;
                            p[2] = (v >> 0) & 0xff;
                            p += 4;
                        }
                        dp += surf->get_stride();
                    }
                    surf->mark_dirty();
                    surf->write_to_png(pic_filename);
                }
            }
        }
    }
    for (const auto &it : pictures_to_delete) {
        Gio::File::create_for_path(build_pic_filename(pic_dir, it, suffix))->remove();
    }
}
} // namespace horizon
