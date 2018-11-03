#include "preview_canvas.hpp"
#include "pool/pool.hpp"
#include "pool/symbol.hpp"
#include "pool/package.hpp"
#include "pool/part.hpp"
#include "board/board_layers.hpp"
#include "preferences/preferences_util.hpp"

namespace horizon {
PreviewCanvas::PreviewCanvas(Pool &p, bool layered) : Glib::ObjectBase(typeid(PreviewCanvas)), CanvasGL(), pool(p)
{
    set_selection_allowed(false);
    preferences_provider_attach_canvas(this, layered);
}

void PreviewCanvas::load_symbol(const UUID &uu, const Placement &pl, bool fit, const UUID &uu_part, const UUID &uu_gate)
{
    std::pair<Coordi, Coordi> bb;
    int64_t pad = 0;
    Symbol sym = *pool.get_symbol(uu);
    sym.expand();
    if (uu_part) {
        auto part = pool.get_part(uu_part);
        const auto &pad_map = part->pad_map;
        for (const auto &it : pad_map) {
            if (it.second.gate->uuid == uu_gate) {
                if (sym.pins.count(it.second.pin->uuid)) {
                    sym.pins.at(it.second.pin->uuid).pad = part->package->pads.at(it.first).name;
                }
            }
        }
    }
    sym.apply_placement(pl);
    update(sym, pl);
    bb = sym.get_bbox();
    pad = 1_mm;

    bb.first.x -= pad;
    bb.first.y -= pad;

    bb.second.x += pad;
    bb.second.y += pad;
    if (fit)
        zoom_to_bbox(bb.first, bb.second);
}

void PreviewCanvas::load(ObjectType type, const UUID &uu, const Placement &pl, bool fit)
{
    std::pair<Coordi, Coordi> bb;
    int64_t pad = 0;
    switch (type) {
    case ObjectType::SYMBOL: {
        Symbol sym = *pool.get_symbol(uu);
        sym.expand();
        sym.apply_placement(pl);
        update(sym, pl);
        bb = sym.get_bbox();
        pad = 1_mm;
    } break;

    case ObjectType::PACKAGE: {
        Package pkg = *pool.get_package(uu);
        for (const auto &la : pkg.get_layers()) {
            auto ld = LayerDisplay::Mode::OUTLINE;
            if (la.second.copper || la.first == BoardLayers::TOP_SILKSCREEN
                || la.first == BoardLayers::BOTTOM_SILKSCREEN)
                ld = LayerDisplay::Mode::FILL_ONLY;
            set_layer_display(la.first, LayerDisplay(true, ld));
        }
        pkg.apply_parameter_set({});
        property_layer_opacity() = 75;
        update(pkg);
        bb = pkg.get_bbox();
        pad = 1_mm;
    } break;

    case ObjectType::PADSTACK: {
        Padstack ps = *pool.get_padstack(uu);
        for (const auto &la : ps.get_layers()) {
            auto ld = LayerDisplay::Mode::OUTLINE;
            if (la.second.copper || la.first == BoardLayers::TOP_SILKSCREEN
                || la.first == BoardLayers::BOTTOM_SILKSCREEN)
                ld = LayerDisplay::Mode::FILL_ONLY;
            set_layer_display(la.first, LayerDisplay(true, ld));
        }
        ps.apply_parameter_set({});
        property_layer_opacity() = 75;
        update(ps);
        bb = ps.get_bbox();
        pad = .1_mm;
        bb.first.x -= pad;
    } break;

    case ObjectType::FRAME: {
        Frame fr = *pool.get_frame(uu);
        property_layer_opacity() = 100;
        update(fr);
        bb = fr.get_bbox();
        pad = 1_mm;
    } break;
    default:;
    }

    bb.first.x -= pad;
    bb.first.y -= pad;

    bb.second.x += pad;
    bb.second.y += pad;
    if (fit)
        zoom_to_bbox(bb.first, bb.second);
}
} // namespace horizon
