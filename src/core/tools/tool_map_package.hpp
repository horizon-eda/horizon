#pragma once
#include "core/tool.hpp"
#include "tool_helper_move.hpp"

namespace horizon {
class ToolMapPackage : public ToolHelperMove {
public:
    ToolMapPackage(IDocument *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;

private:
    std::vector<std::pair<class Component *, bool>> components;
    unsigned int component_index = 0;
    class BoardPackage *pkg = nullptr;
    std::set<UUID> nets;
    void place_package(Component *comp, const Coordi &c);
    void update_tooltip();
    bool flipped = false;
    int angle = 0;
};
} // namespace horizon
