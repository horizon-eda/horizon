#pragma once
#include "canvas/canvas_patch.hpp"
#include "common/common.hpp"
#include "clipper/clipper.hpp"


namespace horizon {
class CanvasMesh {
public:
    void update(const class Board &brd);

    class Layer3D {
    public:
        class Vertex {
        public:
            Vertex(float ix, float iy) : x(ix), y(iy)
            {
            }

            float x, y;
        };
        std::vector<Vertex> tris;
        std::vector<Vertex> walls;
        float offset = 0;
        float thickness = 0.035;
        float alpha = 1;
        float explode_mul = 0;
    };

    const Layer3D &get_layer(int l) const;
    const std::map<int, Layer3D> &get_layers() const;
    const std::map<CanvasPatch::PatchKey, ClipperLib::Paths> &get_patches() const;

private:
    CanvasPatch ca;
    std::map<int, Layer3D> layers;
    const class Board *brd = nullptr;

    void prepare();
    void polynode_to_tris(const ClipperLib::PolyNode *node, int layer);
    void prepare_layer(int layer);
    void prepare_soldermask(int layer);
    void add_path(int layer, const ClipperLib::Path &path);
};
} // namespace horizon
