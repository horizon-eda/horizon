#include "picture_data.hpp"

namespace horizon {
PictureData::PictureData(const UUID &uu, unsigned int w, unsigned int h, std::vector<uint32_t> &&d)
    : uuid(uu), width(w), height(h), data(std::move(d))
{
}
} // namespace horizon
