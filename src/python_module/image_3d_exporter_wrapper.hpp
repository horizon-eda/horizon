#pragma once
#include "export_3d_image/export_3d_image.hpp"

namespace horizon {
class Image3DExporterWrapper : public Image3DExporter {
public:
    using Image3DExporter::Image3DExporter;

    const float &get_center_x() const
    {
        return get_center().x;
    }
    const float &get_center_y() const
    {
        return get_center().y;
    }

    void set_center_x(const float &x)
    {
        set_center({x, get_center().y});
    }
    void set_center_y(const float &y)
    {
        set_center({get_center().x, y});
    }
};
} // namespace horizon
