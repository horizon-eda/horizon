#pragma once
#include "core/tool.hpp"

namespace horizon {
class ToolHelperPlane : public virtual ToolBase {
protected:
    void plane_init(class Polygon &poly);
    [[nodiscard]] bool plane_finish();

private:
    class Plane *plane = nullptr;
};
} // namespace horizon
