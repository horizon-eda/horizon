#pragma once
#include "canvas.hpp"
#include "clipper/clipper.hpp"
#include "util/layer_range.hpp"

namespace horizon {
class CanvasPatch : public Canvas {
public:
    class PatchKey {
    public:
        PatchType type;
        LayerRange layer;
        UUID net;
        bool operator<(const PatchKey &other) const
        {
            if (type < other.type)
                return true;
            else if (type > other.type)
                return false;

            if (layer < other.layer)
                return true;
            else if (layer > other.layer)
                return false;

            return net < other.net;
        }
    };

    const std::map<PatchKey, ClipperLib::Paths> &get_patches() const;
    const std::set<std::tuple<int, Coordi, Coordi>> &get_text_extents() const;
    void clear() override;

    void append_polygon(const Polygon &poly);

    enum class SimplifyOnUpdate { YES, NO };
    CanvasPatch(SimplifyOnUpdate simplify_on_update = SimplifyOnUpdate::YES);

    void push() override
    {
    }
    void request_push() override;
    void simplify();

private:
    const SimplifyOnUpdate simplify_on_update;
    const Net *net = nullptr;
    PatchType patch_type = PatchType::OTHER;
    virtual void img_net(const Net *net) override;
    virtual void img_polygon(const Polygon &poly, bool tr) override;
    virtual void img_polygon(const Polygon &poly, bool tr, const LayerRange &layer);
    virtual void img_hole(const class Hole &hole) override;
    virtual void img_patch_type(PatchType type) override;

    std::map<PatchKey, ClipperLib::Paths> patches;
    std::set<std::tuple<int, Coordi, Coordi>> text_extents;
};
} // namespace horizon
