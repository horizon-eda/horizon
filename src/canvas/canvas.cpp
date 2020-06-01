#include "canvas.hpp"
#include "board/board.hpp"
#include "common/layer_provider.hpp"
#include "schematic/sheet.hpp"
#include "frame/frame.hpp"
#include "util/util.hpp"
#include <algorithm>
#include <iostream>

namespace horizon {

Canvas::Canvas() : selection_filter(this), selectables(this)
{
    layer_display[10000] = LayerDisplay(true, LayerDisplay::Mode::FILL);
    layer_display[0] = LayerDisplay(true, LayerDisplay::Mode::FILL);
}

void Canvas::set_layer_display(int index, const LayerDisplay &ld)
{
    layer_display[index] = ld;
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

void Canvas::clear()
{
    selectables.clear();
    for (auto &it : triangles) {
        if (it.first < 20000 || it.first >= 30000)
            it.second.clear();
    }
    targets.clear();
    sheet_current_uuid = UUID();
    object_refs.clear();
    object_refs_current.clear();
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
        auto begin = triangles[layer].begin();
        auto first = begin + it.second.first;
        auto last = begin + it.second.second + 1;
        assert(it.second.first < triangles[layer].size());
        assert(it.second.second < triangles[layer].size());

        triangles[layer].erase(first, last);
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
            triangles[layer][i].flags |= mask_set;
            triangles[layer][i].flags &= ~mask_clear;
        }
    }
    request_push();
}

void Canvas::set_flags_all(uint8_t mask_set, uint8_t mask_clear)
{
    for (auto &it : triangles) {
        for (auto &it2 : it.second) {
            it2.flags |= mask_set;
            it2.flags &= ~mask_clear;
        }
    }
    request_push();
}

void Canvas::hide_obj(const ObjectRef &r)
{
    set_flags(r, Triangle::FLAG_HIDDEN, 0);
}

void Canvas::show_obj(const ObjectRef &r)
{
    set_flags(r, 0, Triangle::FLAG_HIDDEN);
}

void Canvas::show_all_obj()
{
    set_flags_all(0, Triangle::FLAG_HIDDEN);
}

void Canvas::add_triangle(int layer, const Coordf &p0, const Coordf &p1, const Coordf &p2, ColorP color, uint8_t flags)
{
    triangles[layer].emplace_back(p0, p1, p2, color, triangle_type_current, flags, lod_current);
    if (!fast_draw) {
        auto idx = triangles[layer].size() - 1;
        for (auto &ref : object_refs_current) {
            auto &layers = object_refs[ref];

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
    sheet_current_uuid = sheet.uuid;
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

void Canvas::update(const Buffer &buf, LayerProvider *lp)
{
    clear();
    layer_provider = lp;
    render(buf);
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

int Canvas::get_overlay_layer(int layer, bool ignore_flip)
{
    if (overlay_layers.count({layer, ignore_flip}) == 0) {
        auto ol = overlay_layer_current++;
        overlay_layers[{layer, ignore_flip}] = ol;
        layer_display[ol].visible = true;
        layer_display[ol].mode = LayerDisplay::Mode::OUTLINE;
    }

    return overlay_layers.at({layer, ignore_flip});
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

std::pair<Coordf, Coordf> Canvas::get_bbox(bool visible_only) const
{
    Coordf a, b;
    for (const auto &it : triangles) {
        if (visible_only == false || get_layer_display(it.first).visible) {
            for (const auto &it2 : it.second) {
                if (it2.flags & Triangle::FLAG_GLYPH)
                    continue;
                std::vector<Coordf> points = {Coordf(it2.x0, it2.y0), Coordf(it2.x1, it2.y2), Coordf(it2.x1, it2.y2)};
                if (std::isnan(it2.y2)) { // line
                    float width = it2.x2;
                    Coordf offset(width / 2, width / 2);
                    a = Coordf::min(a, points.at(0) - offset);
                    a = Coordf::min(a, points.at(1) - offset);
                    b = Coordf::max(b, points.at(0) + offset);
                    b = Coordf::max(b, points.at(1) + offset);
                }
                else { // triangle
                    for (const auto &p : points) {
                        a = Coordf::min(a, p);
                        b = Coordf::max(b, p);
                    }
                }
            }
        }
    }
    if (sqrt((b - a).mag_sq()) < .1_mm)
        return std::make_pair(Coordf(-5_mm, -5_mm), Coordf(5_mm, 5_mm));
    else
        return std::make_pair(a, b);
}
} // namespace horizon
