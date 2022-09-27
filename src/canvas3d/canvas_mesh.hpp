#pragma once
#include "canvas/canvas_patch.hpp"
#include "common/common.hpp"
#include "clipper/clipper.hpp"
#include <atomic>

namespace horizon {
class CanvasMesh {
public:
    void update(const class Board &brd);
    void update_only(const class Board &brd);
    void prepare_only(std::function<void()> cb = nullptr);

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
        void move_from(Layer3D &&other);
        void copy_sizes_from(const Layer3D &other);

        float offset = 0;
        float thickness = 0.035;
        float alpha = 1;
        float explode_mul = 0;
        float specular_intensity = 0.3;
        float specular_power = 16;
        float ambient_intensity = .3;
        float diffuse_intensity = 0.8;

        std::atomic_bool done = false;
    };

    std::map<int, Layer3D> &get_layers()
    {
        return layers;
    }
    const std::map<int, Layer3D> &get_layers() const
    {
        return layers;
    }
    std::pair<Coordi, Coordi> get_bbox() const;
    CanvasMesh();
    void cancel_prepare()
    {
        cancel = true;
    }

private:
    CanvasPatch ca;
    std::map<int, Layer3D> layers;
    std::atomic_bool cancel = false;

    void prepare(const class Board &brd);
    void prepare_work(std::function<void()> cb);
    void polynode_to_tris(const ClipperLib::PolyNode *node, int layer);
    std::vector<int> layers_to_prepare;

    void prepare_worker(std::atomic_size_t &layer_counter, std::function<void()> cb);
    void prepare_layer(int layer);
    void prepare_soldermask(int layer);
    void prepare_silkscreen(int layer, int soldermask_layer);
    void add_path(int layer, const ClipperLib::Path &path);
};
} // namespace horizon
