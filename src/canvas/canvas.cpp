#include "canvas.hpp"
#include "board/board.hpp"
#include "common/layer_provider.hpp"
#include "schematic/sheet.hpp"
#include "frame/frame.hpp"
#include "pool/decal.hpp"
#include "util/util.hpp"
#include <algorithm>
#include "util/bbox_accumulator.hpp"
#include "block_symbol/block_symbol.hpp"

namespace horizon {

Canvas::Canvas() : selectables(*this)
{
    layer_display[10000] = LayerDisplay(true, LayerDisplay::Mode::FILL);
    layer_display[0] = LayerDisplay(true, LayerDisplay::Mode::FILL);
}

void Canvas::set_layer_display(int index, const LayerDisplay &ld)
{
    layer_display[index] = ld;
    request_push();
}

static const LayerDisplay ld_default;

const LayerDisplay &Canvas::get_layer_display(int index) const
{
    if (layer_display.count(index))
        return layer_display.at(index);
    else
        return ld_default;
}

bool Canvas::layer_is_visible(int layer) const
{
    return layer == work_layer || get_layer_display(layer).visible;
}

bool Canvas::layer_is_visible(LayerRange layer) const
{
    if (!layer.is_multilayer())
        return layer_is_visible(layer.start());
    if (layer.overlaps(work_layer))
        return true;
    for (auto la : {layer.start(), layer.end()}) {
        if (la == work_layer || get_layer_display(la).visible)
            return true;
    }
    for (const auto &[la, ld] : layer_display) {
        if (ld.visible && layer.overlaps(la))
            return true;
    }
    return false;
}

void Canvas::clear()
{
    selectables.clear();
    for (auto &it : triangles) {
        if (it.first < 20000 || it.first >= 30000)
            it.second.clear();
    }
    targets.clear();
    for (auto &it : object_refs) {
        it.second.clear();
    }
    object_refs_current.clear();
    object_ref_idx.clear();
    pictures.clear();
}

void Canvas::remove_obj(const ObjectRef &r)
{
    if (!object_refs.count(r))
        return;
    std::set<int> layers;
    for (const auto &it : object_refs.at(r)) {
        auto layer = it.first;
        layers.insert(layer);
        auto first = it.second.first;
        auto last = it.second.second + 1;
        assert(it.second.first < triangles[layer].size());
        assert(it.second.second < triangles[layer].size());

        triangles.at(layer).erase(first, last);
    }

    // fix refs that changed due to triangles being deleted
    for (auto &it : object_refs) {
        if (it.first != r) {
            for (auto layer : layers) {
                if (it.second.count(layer)) {
                    auto &idx = it.second.at(layer);
                    auto idx_removed = object_refs.at(r).at(layer);
                    auto n_removed = (idx_removed.second - idx_removed.first) + 1;
                    if (idx.first > idx_removed.first) {
                        idx.first -= n_removed;
                        idx.second -= n_removed;
                    }
                }
            }
        }
    }

    object_refs.erase(r);

    request_push();
}

void Canvas::set_flags(const ObjectRef &r, uint8_t mask_set, uint8_t mask_clear)
{
    if (!object_refs.count(r))
        return;
    for (const auto &it : object_refs.at(r)) {
        auto layer = it.first;
        for (auto i = it.second.first; i <= it.second.second; i++) {
            triangles.at(layer).atm(i).second.flags |= mask_set;
            triangles.at(layer).atm(i).second.flags &= ~mask_clear;
        }
    }
    request_push();
}

void Canvas::set_flags_all(uint8_t mask_set, uint8_t mask_clear)
{
    for (auto &it : triangles) {
        for (auto it2 : it.second) {
            it2.second.flags |= mask_set;
            it2.second.flags &= ~mask_clear;
        }
    }
    request_push();
}

void Canvas::hide_obj(const ObjectRef &r)
{
    set_flags(r, TriangleInfo::FLAG_HIDDEN, 0);
}

void Canvas::show_obj(const ObjectRef &r)
{
    set_flags(r, 0, TriangleInfo::FLAG_HIDDEN);
}

void Canvas::show_all_obj()
{
    set_flags_all(0, TriangleInfo::FLAG_HIDDEN);
}

void Canvas::reset_color2()
{
    for (auto &[layer, tris] : triangles) {
        for (auto [tri, tri_info] : tris) {
            tri.color2 = 0;
        }
    }
    request_push();
}

void Canvas::set_color2(const ObjectRef &r, uint8_t color)
{
    if (!object_refs.count(r))
        return;
    for (const auto &[layer, range] : object_refs.at(r)) {
        const auto [first, last] = range;
        for (auto i = first; i <= last; i++) {
            triangles.at(layer).atm(i).first.color2 = color;
        }
    }
}

void Canvas::add_triangle(int layer, const Coordf &p0, const Coordf &p1, const Coordf &p2, ColorP color, uint8_t flags,
                          uint8_t color2)
{
    if (group_tris) {
        assert(group_layer == layer);
        group_tris->emplace_back(std::forward_as_tuple(p0, p1, p2, color, lod_current, color2),
                                 std::forward_as_tuple(triangle_type_current, flags));
        return;
    }
    triangles[layer].emplace_back(std::forward_as_tuple(p0, p1, p2, color, lod_current, color2),
                                  std::forward_as_tuple(triangle_type_current, flags));
    auto idx = triangles[layer].size() - 1;
    for (auto it : object_ref_idx) {
        auto &layers = *it;
        if (layers.count(layer)) {
            auto &idxs = layers[layer];
            assert(idxs.second == idx - 1);
            idxs.second = idx;
        }
        else {
            auto &idxs = layers[layer];
            idxs.first = idx;
            idxs.second = idx;
        }
    }
}

void Canvas::update(const Symbol &sym, const Placement &tr, bool edit)
{
    clear();
    layer_provider = &sym;
    transform = tr;
    render(sym, !edit);
    request_push();
}

void Canvas::update(const Sheet &sheet)
{
    clear();
    layer_provider = &sheet;
    update_markers();
    render(sheet);
    request_push();
}

void Canvas::update(const Padstack &padstack, bool edit)
{
    clear();
    layer_provider = &padstack;
    render(padstack, edit);
    request_push();
}

void Canvas::update(const Package &pkg, bool edit)
{
    clear();
    layer_provider = &pkg;
    render(pkg, edit);
    request_push();
}

void Canvas::update(const Board &brd, PanelMode mode)
{
    clear();
    layer_provider = &brd;
    render(brd, true, mode);
    request_push();
}
void Canvas::update(const class Frame &fr, bool edit)
{
    clear();
    layer_provider = &fr;
    render(fr, !edit);
    request_push();
}

void Canvas::update(const class Decal &dec, bool edit)
{
    clear();
    layer_provider = &dec;
    render(dec, edit);
    request_push();
}

void Canvas::update(const class BlockSymbol &sym, bool edit)
{
    clear();
    layer_provider = &sym;
    render(sym, !edit);
    request_push();
}

void Canvas::transform_save()
{
    transforms.push_back(transform);
}

void Canvas::transform_restore()
{
    if (transforms.size()) {
        transform = transforms.back();
        transforms.pop_back();
    }
}

int Canvas::get_overlay_layer(const LayerRange &layer, bool ignore_flip)
{
    if (overlay_layers.count({layer, ignore_flip}) == 0) {
        auto ol = overlay_layer_current++;
        overlay_layers[{layer, ignore_flip}] = ol;
        layer_display[ol].visible = true;
        layer_display[ol].mode = LayerDisplay::Mode::OUTLINE;
    }

    return overlay_layers.at({layer, ignore_flip});
}

bool Canvas::is_overlay_layer(int overlay_layer, int layer) const
{
    for (const auto &[k, v] : overlay_layers) {
        if (k.first.overlaps(layer))
            if (v == overlay_layer)
                return true;
    }
    return false;
}


Color Canvas::get_layer_color(int layer) const
{
    if (layer_colors.count(layer))
        return layer_colors.at(layer);
    else
        return {1, 1, 0};
}

void Canvas::set_layer_color(int layer, const Color &color)
{
    layer_colors[layer] = color;
}

void Canvas::object_ref_pop()
{
    object_refs_current.pop_back();
    object_ref_idx.pop_back();
    assert(group_tris == nullptr);
}

void Canvas::object_ref_push(const ObjectRef &ref)
{
    object_refs_current.push_back(ref);
    object_ref_idx.push_back(&object_refs[ref]);
    assert(group_tris == nullptr);
}

void Canvas::begin_group(int layer)
{
    group_layer = layer;
    assert(group_tris == nullptr);
    group_tris = &triangles[layer];
    group_size = group_tris->size();
}

void Canvas::end_group()
{
    auto new_size = group_tris->size();
    if (new_size != group_size) { // something was drawn
        for (auto it : object_ref_idx) {
            auto &layers = *it;
            if (layers.count(group_layer)) {
                auto &idxs = layers[group_layer];
                idxs.second = new_size - 1;
            }
            else {
                auto &idxs = layers[group_layer];
                idxs.first = group_size;
                idxs.second = new_size - 1;
            }
        }
    }
    group_tris = nullptr;
}

std::pair<Coordf, Coordf> Canvas::get_bbox(bool visible_only) const
{
    BBoxAccumulator<Coordf::type> acc;
    for (const auto &it : triangles) {
        if (visible_only == false || get_layer_display(it.first).visible) {
            for (const auto &[it2, it_info] : it.second) {
                if (it_info.flags & TriangleInfo::FLAG_GLYPH)
                    continue;
                const std::vector<Coordf> points = {Coordf(it2.x0, it2.y0), Coordf(it2.x1, it2.y1),
                                                    Coordf(it2.x2, it2.y2)};
                if (it_info.flags & TriangleInfo::FLAG_ARC) {
                    const float r = it2.x2 + it2.y2 / 2;
                    const auto rv = Coordf(r, r);
                    acc.accumulate({points.at(0) - rv, points.at(0) + rv});
                }
                else if (std::isnan(it2.y1) && std::isnan(it2.x2) && std::isnan(it2.y2)) { // circle
                    auto r = Coordf(it2.x1, it2.x1);
                    acc.accumulate({points.at(0) - r, points.at(0) + r});
                }
                else if (std::isnan(it2.y2) && !(it_info.flags & TriangleInfo::FLAG_BUTT)) { // line
                    float width = it2.x2;
                    Coordf offset(width / 2, width / 2);
                    acc.accumulate(points.at(0) - offset);
                    acc.accumulate(points.at(1) - offset);
                    acc.accumulate(points.at(0) + offset);
                    acc.accumulate(points.at(1) + offset);
                }
                else if (it_info.flags & TriangleInfo::FLAG_BUTT) { // line
                    const Coordf v = points.at(1) - points.at(0);
                    const Coordf vn = Coordf(-v.y, v.x).normalize();
                    const Coordf w = vn * (it2.x2 / 2);
                    acc.accumulate(points.at(0) - w);
                    acc.accumulate(points.at(1) - w);
                    acc.accumulate(points.at(0) + w);
                    acc.accumulate(points.at(1) + w);
                }
                else { // triangle
                    for (const auto &p : points) {
                        acc.accumulate(p);
                    }
                }
            }
        }
    }
    const auto [a, b] = acc.get_or_0();
    if ((b - a).mag() < .1_mm)
        return std::make_pair(Coordf(-5_mm, -5_mm), Coordf(5_mm, 5_mm));
    else
        return std::make_pair(a, b);
}
} // namespace horizon
