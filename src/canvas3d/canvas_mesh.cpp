#include "canvas_mesh.hpp"
#include "board/board.hpp"
#include "board/board_layers.hpp"
#include "poly2tri/poly2tri.h"
#include "logger/logger.hpp"
#include "util/util.hpp"

namespace horizon {
void CanvasMesh::update(const Board &b)
{
    brd = &b;
    ca.update(b);
    prepare();
}

const CanvasMesh::Layer3D &CanvasMesh::get_layer(int l) const
{
    return layers.at(l);
}

const std::map<int, CanvasMesh::Layer3D> &CanvasMesh::get_layers() const
{
    return layers;
}

void CanvasMesh::prepare()
{
    if (!brd)
        return;
    layers.clear();

    float board_thickness = -((float)brd->stackup.at(0).thickness);
    int n_inner_layers = brd->get_n_inner_layers();
    for (const auto &it : brd->stackup) {
        board_thickness += it.second.thickness + it.second.substrate_thickness;
    }
    board_thickness /= 1e6;

    int layer = BoardLayers::TOP_COPPER;
    layers[layer].offset = 0;
    layers[layer].thickness = brd->stackup.at(0).thickness / 1e6;
    layers[layer].explode_mul = 1;
    prepare_layer(layer);

    layer = BoardLayers::BOTTOM_COPPER;
    layers[layer].offset = -board_thickness;
    layers[layer].thickness = +(brd->stackup.at(layer).thickness / 1e6);
    layers[layer].explode_mul = -2 * n_inner_layers - 1;
    prepare_layer(layer);

    {
        float offset = -(brd->stackup.at(0).substrate_thickness / 1e6);
        for (int i = 0; i < n_inner_layers; i++) {
            layer = -i - 1;
            layers[layer].offset = offset;
            layers[layer].thickness = -(brd->stackup.at(layer).thickness / 1e6);
            layers[layer].explode_mul = -1 - 2 * i;
            offset -= brd->stackup.at(layer).thickness / 1e6 + brd->stackup.at(layer).substrate_thickness / 1e6;
            prepare_layer(layer);
        }
    }

    layer = BoardLayers::L_OUTLINE;
    layers[layer].offset = 0;
    layers[layer].thickness = -(brd->stackup.at(0).substrate_thickness / 1e6);
    layers[layer].explode_mul = 0;
    prepare_layer(layer);

    float offset = -(brd->stackup.at(0).substrate_thickness / 1e6);
    for (int i = 0; i < n_inner_layers; i++) {
        int l = 10000 + i;
        offset -= brd->stackup.at(-i - 1).thickness / 1e6;
        layers[l] = layers[layer];
        layers[l].offset = offset;
        layers[l].thickness = -(brd->stackup.at(-i - 1).substrate_thickness / 1e6);
        layers[l].explode_mul = -2 - 2 * i;

        offset -= brd->stackup.at(-i - 1).substrate_thickness / 1e6;
    }

    layer = BoardLayers::TOP_MASK;
    layers[layer].offset = brd->stackup.at(0).thickness / 1e6 + 1e-3;
    layers[layer].thickness = 0.01;
    layers[layer].alpha = .8;
    layers[layer].explode_mul = 3;
    prepare_soldermask(layer);

    layer = BoardLayers::BOTTOM_MASK;
    layers[layer].offset = -board_thickness - 1e-3;
    layers[layer].thickness = 0.035;
    layers[layer].alpha = .8;
    layers[layer].explode_mul = -2 * n_inner_layers - 3;
    prepare_soldermask(layer);

    layer = BoardLayers::TOP_SILKSCREEN;
    layers[layer].offset = brd->stackup.at(0).thickness / 1e6 + 1e-3;
    layers[layer].thickness = 0.035;
    layers[layer].explode_mul = 4;
    prepare_layer(layer);

    layer = BoardLayers::BOTTOM_SILKSCREEN;
    layers[layer].offset = -board_thickness - .1e-3;
    layers[layer].thickness = -0.035;
    layers[layer].explode_mul = -2 * n_inner_layers - 4;
    prepare_layer(layer);

    layer = BoardLayers::TOP_PASTE;
    layers[layer].offset = brd->stackup.at(0).thickness / 1e6 + 1e-3;
    layers[layer].thickness = 0.035;
    layers[layer].explode_mul = 2;
    prepare_layer(layer);

    layer = BoardLayers::BOTTOM_PASTE;
    layers[layer].offset = -board_thickness;
    layers[layer].thickness = -0.035;
    layers[layer].explode_mul = -2 * n_inner_layers - 2;
    prepare_layer(layer);

    layer = 20000; // pth holes
    layers[layer].offset = 0;
    layers[layer].thickness = -board_thickness;

    for (const auto &it : ca.get_patches()) {
        if (it.first.layer == 10000 && it.first.type == PatchType::HOLE_PTH) {
            ClipperLib::ClipperOffset ofs;
            ofs.AddPaths(it.second, ClipperLib::jtRound, ClipperLib::etClosedPolygon);
            ClipperLib::Paths res;
            ofs.Execute(res, -.001_mm);
            for (const auto &path : res) {
                add_path(layer, path);
            }
        }
    }
}


void CanvasMesh::prepare_soldermask(int layer)
{
    ClipperLib::Paths temp;
    {
        ClipperLib::Clipper cl;
        for (const auto &it : ca.get_patches()) {
            if (it.first.layer == BoardLayers::L_OUTLINE) { // add outline
                cl.AddPaths(it.second, ClipperLib::ptSubject, true);
            }
            else if (it.first.layer == layer) {
                cl.AddPaths(it.second, ClipperLib::ptClip, true);
            }
        }

        cl.Execute(ClipperLib::ctDifference, temp, ClipperLib::pftEvenOdd, ClipperLib::pftNonZero);
    }
    ClipperLib::PolyTree pt;
    ClipperLib::ClipperOffset cl;
    cl.AddPaths(temp, ClipperLib::jtSquare, ClipperLib::etClosedPolygon);
    cl.Execute(pt, -.001_mm);

    for (const auto node : pt.Childs) {
        polynode_to_tris(node, layer);
    }
}

void CanvasMesh::prepare_layer(int layer)
{
    ClipperLib::Clipper cl;
    for (const auto &it : ca.get_patches()) {
        if (it.first.layer == layer) {
            cl.AddPaths(it.second, ClipperLib::ptSubject, true);
        }
    }
    ClipperLib::Paths result;
    auto pft = ClipperLib::pftNonZero;
    if (layer == BoardLayers::L_OUTLINE) {
        pft = ClipperLib::pftEvenOdd;
    }
    cl.Execute(ClipperLib::ctUnion, result, pft);

    ClipperLib::PolyTree pt;
    cl.Clear();
    cl.AddPaths(result, ClipperLib::ptSubject, true);
    for (const auto &it : ca.get_patches()) {
        if (it.first.layer == 10000
            && (it.first.type == PatchType::HOLE_NPTH || it.first.type == PatchType::HOLE_PTH)) {
            cl.AddPaths(it.second, ClipperLib::ptClip, true);
        }
    }
    cl.Execute(ClipperLib::ctDifference, pt, pft, ClipperLib::pftNonZero);

    for (const auto node : pt.Childs) {
        polynode_to_tris(node, layer);
    }
}


void CanvasMesh::add_path(int layer, const ClipperLib::Path &path)
{
    if (path.size() >= 3) {
        layers[layer].walls.emplace_back(path.back().X, path.back().Y);
        for (size_t i = 0; i < path.size(); i++) {
            layers[layer].walls.emplace_back(path[i].X, path[i].Y);
        }
        layers[layer].walls.emplace_back(path[0].X, path[0].Y);
        layers[layer].walls.emplace_back(path[1].X, path[1].Y);
        layers[layer].walls.emplace_back(NAN, NAN);
    }
}

static void append_path(std::vector<p2t::Point> &store, std::vector<p2t::Point *> &out,
                        std::set<std::pair<ClipperLib::cInt, ClipperLib::cInt>> &point_set,
                        const ClipperLib::Path &path)
{
    for (const auto &it : path) {
        auto p = std::make_pair(it.X, it.Y);
        bool a = false;
        bool fixed = false;
        while (point_set.count(p)) {
            fixed = true;
            if (a)
                p.first++;
            else
                p.second++;
            a = !a;
        }
        if (fixed) {
            Logger::log_warning("fixed duplicate point", Logger::Domain::BOARD,
                                "at " + coord_to_string(Coordf(it.X, it.Y)));
        }
        point_set.insert(p);
        store.emplace_back(p.first, p.second);
        out.push_back(&store.back());
    }
}

void CanvasMesh::polynode_to_tris(const ClipperLib::PolyNode *node, int layer)
{
    assert(node->IsHole() == false);

    std::vector<p2t::Point> point_store;
    size_t pts_total = node->Contour.size();
    for (const auto child : node->Childs)
        pts_total += child->Contour.size();
    point_store.reserve(pts_total); // important so that iterators won't get invalidated
    std::set<std::pair<ClipperLib::cInt, ClipperLib::cInt>> point_set;

    try {
        std::vector<p2t::Point *> contour;
        contour.reserve(node->Contour.size());
        append_path(point_store, contour, point_set, node->Contour);
        p2t::CDT cdt(contour);
        for (const auto child : node->Childs) {
            std::vector<p2t::Point *> hole;
            hole.reserve(child->Contour.size());
            append_path(point_store, hole, point_set, child->Contour);
            cdt.AddHole(hole);
        }
        cdt.Triangulate();
        auto tris = cdt.GetTriangles();

        for (const auto &tri : tris) {
            for (int i = 0; i < 3; i++) {
                auto p = tri->GetPoint(i);
                layers[layer].tris.emplace_back(p->x, p->y);
            }
        }
    }
    catch (const std::runtime_error &e) {
        Logger::log_critical("error triangulating layer " + brd->get_layers().at(layer).name, Logger::Domain::BOARD,
                             e.what());
    }
    catch (...) {
        Logger::log_critical("error triangulating layer" + brd->get_layers().at(layer).name, Logger::Domain::BOARD,
                             "unspecified error");
    }

    layers[layer].walls.reserve(pts_total);
    add_path(layer, node->Contour);
    for (auto child : node->Childs) {
        add_path(layer, child->Contour);
    }

    for (auto child : node->Childs) {
        assert(child->IsHole() == true);
        for (auto child2 : child->Childs) { // add fragments in holes
            polynode_to_tris(child2, layer);
        }
    }
}

const std::map<CanvasPatch::PatchKey, ClipperLib::Paths> &CanvasMesh::get_patches() const
{
    return ca.get_patches();
}

} // namespace horizon
