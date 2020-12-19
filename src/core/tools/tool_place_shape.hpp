#pragma once
#include "common/shape.hpp"
#include "core/tool.hpp"
#include <forward_list>

namespace horizon {

class ToolPlaceShape : public ToolBase {
public:
    using ToolBase::ToolBase;
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::LMB, I::CANCEL, I::RMB, I::EDIT, I::ROTATE,
        };
    }

protected:
    Shape *temp = 0;
    std::forward_list<Shape *> shapes_placed;

    void create_shape(const Coordi &c);
};
} // namespace horizon
