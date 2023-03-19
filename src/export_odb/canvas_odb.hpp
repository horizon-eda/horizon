#pragma once
#include "canvas/canvas.hpp"
#include "db.hpp"

namespace horizon {
class CanvasODB : public Canvas {
public:
    CanvasODB(ODB::Job &job, const class Board &brd);
    void push() override
    {
    }
    void request_push() override;
    uint64_t outline_width = 0;

    std::map<int, ODB::Features *> layer_features;
    std::map<LayerRange, ODB::Features *> drill_features;
    ODB::EDAData *eda_data = nullptr;

    std::map<std::pair<UUID, UUID>, ODB::EDAData::SubnetToeprint *> pad_subnets;
    std::map<UUID, ODB::EDAData::SubnetTrace *> track_subnets;

private:
    void img_net(const Net *net) override;
    void img_polygon(const Polygon &poly, bool tr) override;
    void img_arc(const Coordi &from, const Coordi &to, const Coordi &center, const uint64_t width, int layer) override;
    void img_line(const Coordi &p0, const Coordi &p1, const uint64_t width, int layer, bool tr = true) override;
    void img_padstack(const Padstack &ps) override;
    void img_hole(const Hole &hole) override;
    void img_set_padstack(bool v) override;
    void img_patch_type(PatchType pt) override;
    void img_text(const Text *text) override;

    PatchType patch_type = PatchType::OTHER;
    const Text *text_current = nullptr;

    bool padstack_mode = false;

    ODB::Features *get_layer_features(int layer)
    {
        auto x = layer_features.find(layer);
        if (x == layer_features.end())
            return nullptr;
        else
            return x->second;
    }

    ODB::Job &job;
    const Board &brd;

    std::map<UUID, ODB::EDAData::SubnetVia *> via_subnets;

    ODB::EDAData::SubnetToeprint *get_subnet_toeprint();
};
} // namespace horizon
