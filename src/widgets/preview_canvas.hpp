#pragma once
#include "canvas/canvas_gl.hpp"

namespace horizon {
class PreviewCanvas : public CanvasGL {
public:
    PreviewCanvas(class Pool &pool, bool layered);
    void load(ObjectType ty, const UUID &uu, const Placement &pl = Placement(), bool fit = true);
    void load_symbol(const UUID &uu, const Placement &pl = Placement(), bool fit = true, const UUID &uu_part = UUID(),
                     const UUID &uu_gate = UUID());

private:
    class Pool &pool;
};
} // namespace horizon
