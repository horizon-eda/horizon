#pragma once
#include <memory>
#include "util/picture_data.hpp"

namespace horizon {
class CanvasPicture {
public:
    float x;
    float y;
    float angle;
    float px_size;
    bool on_top;
    float opacity;
    std::shared_ptr<const PictureData> data;
};
} // namespace horizon
