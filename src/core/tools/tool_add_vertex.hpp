#pragma once
#include "common/polygon.hpp"
#include "core/tool.hpp"

namespace horizon {

class ToolAddVertex : public ToolBase {
public:
    using ToolBase::ToolBase;
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    bool is_specific() override
    {
        return true;
    }
    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::LMB,
                I::CANCEL,
                I::RMB,
                I::FLIP_DIRECTION,
        };
    }

private:
    Polygon *poly = nullptr;
    Polygon::Vertex *vertex = nullptr;
    int vertex_index = 0;
    unsigned int n_vertices_placed = 0;
    bool flip_direction = false;
    void update_tip();
    void add_vertex(const Coordi &c);
};
} // namespace horizon
