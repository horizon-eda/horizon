#pragma once
#include "canvas/canvas_gl.hpp"

namespace horizon {
class PreviewCanvas : public CanvasGL {
public:
    PreviewCanvas(class Pool &pool, bool layered);
    void load(ObjectType ty, const UUID &uu, const Placement &pl = Placement(), bool fit = true);

private:
    class Pool &pool;
};
} // namespace horizon
