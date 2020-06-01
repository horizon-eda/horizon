#pragma once
#include <stdint.h>
#include <vector>
#include "util/uuid.hpp"

namespace horizon {
class PictureData {
public:
    PictureData(const UUID &uu, unsigned int w, unsigned int h, std::vector<uint32_t> &&d);
    const UUID uuid;
    const unsigned int width;
    const unsigned int height;
    const std::vector<uint32_t> data;
};
} // namespace horizon
