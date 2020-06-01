#pragma once
#include <memory>
#include <string>
#include <gdkmm.h>

namespace horizon {
std::shared_ptr<const class PictureData> picture_data_from_file(const std::string &filename);
std::shared_ptr<const class PictureData> picture_data_from_pixbuf(Glib::RefPtr<Gdk::Pixbuf> pixbuf);
} // namespace horizon
