#include "canvas_patch.hpp"
#include "board/plane.hpp"
#include "common/hole.hpp"
#include "board/board_layers.hpp"
#include "util/clipper_util.hpp"
#include <atomic>
#include <future>
#include <thread>

namespace horizon {
CanvasPatch::CanvasPatch(SimplifyOnUpdate simp) : Canvas::Canvas(), simplify_on_update(simp)
{
    img_mode = true;
}

static void simplify_worker(std::vector<ClipperLib::Paths *> patches, std::atomic_size_t &patch_counter)
{
    size_t i;
    const auto n = patches.size();
    while ((i = patch_counter.fetch_add(1, std::memory_order_relaxed)) < n) {
        ClipperLib::SimplifyPolygons(*patches.at(i), ClipperLib::pftNonZero);
    }
}

void CanvasPatch::simplify()
{
    std::vector<ClipperLib::Paths *> patches_to_simplify;
    patches_to_simplify.reserve(patches.size());

    for (auto &[key, patch] : patches) {
        if (key.layer != BoardLayers::L_OUTLINE)
            patches_to_simplify.push_back(&patch);
    }
    {
        std::atomic_size_t patch_counter = 0;
        std::vector<std::future<void>> results;
        for (size_t i = 0; i < std::thread::hardware_concurrency(); i++) {
            results.push_back(std::async(std::launch::async, simplify_worker, std::ref(patches_to_simplify),
                                         std::ref(patch_counter)));
        }
        for (auto &it : results) {
            it.wait();
        }
    }
}

void CanvasPatch::request_push()
{
    if (simplify_on_update == SimplifyOnUpdate::YES)
        simplify();
}

void CanvasPatch::img_net(const Net *n)
{
    net = n;
}

void CanvasPatch::img_patch_type(PatchType pt)
{
    patch_type = pt;
}

void CanvasPatch::img_hole(const Hole &hole)
{
    // create patch of type HOLE_PTH/HOLE_NPTH on layer 10000
    // for NPTH, set net to 0
    auto net_saved = net;
    auto patch_type_saved = patch_type;
    if (!hole.plated) {
        net = nullptr;
        patch_type = PatchType::HOLE_NPTH;
    }
    else {
        patch_type = PatchType::HOLE_PTH;
    }
    auto poly = hole.to_polygon().remove_arcs(64);
    img_polygon(poly, true, hole.span);
    net = net_saved;
    patch_type = patch_type_saved;
}

void CanvasPatch::append_polygon(const Polygon &poly)
{
    img_polygon(poly, false);
}

void CanvasPatch::img_polygon(const Polygon &ipoly, bool tr)
{
    img_polygon(ipoly, tr, ipoly.layer);
}

void CanvasPatch::img_polygon(const Polygon &ipoly, bool tr, const LayerRange &layer)
{
    if (!img_layer_is_visible(layer))
        return;
    auto poly = ipoly.remove_arcs(64);
    if (poly.usage == nullptr) { // regular patch
        PatchKey patch_key{patch_type, layer, net ? net->uuid : UUID()};
        patches[patch_key];

        patches[patch_key].emplace_back();

        ClipperLib::Path &t = patches[patch_key].back();
        t.reserve(poly.vertices.size());
        if (tr) {
            for (const auto &it : poly.vertices) {
                auto p = transform.transform(it.position);
                t.emplace_back(p.x, p.y);
            }
        }
        else {
            for (const auto &it : poly.vertices) {
                t.emplace_back(it.position.x, it.position.y);
            }
        }
        if (ClipperLib::Orientation(t)) {
            std::reverse(t.begin(), t.end());
        }
    }
    else if (auto plane = dynamic_cast<Plane *>(poly.usage.ptr)) {
        PatchKey patch_key{PatchType::PLANE, layer, plane->net ? plane->net->uuid : UUID()};
        patches[patch_key];

        for (const auto &frag : plane->fragments) {
            patches[patch_key].emplace_back();
            ClipperLib::Path &contour = patches[patch_key].back();
            contour = transform_path(transform, frag.paths.front());

            bool needs_reverse = false;
            if (ClipperLib::Orientation(contour)) {
                std::reverse(contour.begin(), contour.end());
                needs_reverse = true;
            }

            for (size_t i = 1; i < frag.paths.size(); i++) {
                patches[patch_key].emplace_back();
                ClipperLib::Path &hole = patches[patch_key].back();
                hole = transform_path(transform, frag.paths[i]);
                if (needs_reverse) {
                    std::reverse(hole.begin(), hole.end());
                }
            }
        }
    }
}

const std::map<CanvasPatch::PatchKey, ClipperLib::Paths> &CanvasPatch::get_patches() const
{
    return patches;
}

const std::set<std::tuple<int, Coordi, Coordi>> &CanvasPatch::get_text_extents() const
{
    return text_extents;
}

void CanvasPatch::clear()
{
    patches.clear();
    text_extents.clear();
    Canvas::clear();
}

/*void CanvasPatch::img_text_extents(const Text &txt, std::pair<Coordf, Coordf> &extents)
{
    std::pair<Coordi, Coordi> ext(Coordi(extents.first.x, extents.first.y), Coordi(extents.second.x, extents.second.y));
    text_extents.emplace(txt.layer, ext.first, ext.second);
}*/
} // namespace horizon
