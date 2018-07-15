#include "preview_canvas.hpp"
#include "pool/pool.hpp"
#include "pool/symbol.hpp"
#include "pool/package.hpp"
#include "board/board_layers.hpp"

namespace horizon {
PreviewCanvas::PreviewCanvas(Pool &p) : Glib::ObjectBase(typeid(PreviewCanvas)), CanvasGL(), pool(p)
{
    set_selection_allowed(false);
}

void PreviewCanvas::load(ObjectType type, const UUID &uu, const Placement &pl, bool fit)
{
    std::pair<Coordi, Coordi> bb;
    int64_t pad = 0;
    switch (type) {
    case ObjectType::SYMBOL: {
        Symbol sym = *pool.get_symbol(uu);
        for (const auto &la : sym.get_layers()) {
            set_layer_display(la.first, LayerDisplay(true, LayerDisplay::Mode::FILL, la.second.color));
        }
        sym.expand();
        sym.apply_placement(pl);
        update(sym, pl);
        bb = sym.get_bbox();
    } break;

    case ObjectType::PACKAGE: {
        Package pkg = *pool.get_package(uu);
        for (const auto &la : pkg.get_layers()) {
            auto ld = LayerDisplay::Mode::OUTLINE;
            if (la.second.copper || la.first == BoardLayers::TOP_SILKSCREEN
                || la.first == BoardLayers::BOTTOM_SILKSCREEN)
                ld = LayerDisplay::Mode::FILL_ONLY;
            set_layer_display(la.first, LayerDisplay(true, ld, la.second.color));
        }
        pkg.apply_parameter_set({});
        property_layer_opacity() = 75;
        update(pkg);
        bb = pkg.get_bbox();
    } break;
    default:;
    }

    pad = std::max(bb.second.x, bb.second.y);
    pad /= 1.61803;

    bb.first.x -= pad;
    bb.first.y -= pad;

    bb.second.x += pad;
    bb.second.y += pad;
    if (fit)
        zoom_to_bbox(bb.first, bb.second);
}
} // namespace horizon
