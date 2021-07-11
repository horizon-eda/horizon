#pragma once
#include "core/tool.hpp"

namespace horizon {
class ToolMapPort : public ToolBase {
public:
    using ToolBase::ToolBase;
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    ~ToolMapPort();
    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::LMB, I::CANCEL, I::RMB, I::ROTATE, I::MIRROR, I::EDIT, I::AUTOPLACE_ALL_PINS, I::AUTOPLACE_NEXT_PIN,
        };
    }

private:
    std::vector<std::pair<const class Net *, bool>> ports;
    unsigned int port_index = 0;
    class BlockSymbolPort *port = nullptr;
    BlockSymbolPort *port_last = nullptr;
    BlockSymbolPort *port_last2 = nullptr;
    void create_port(const UUID &uu);
    bool can_autoplace() const;
    void update_tip();
    class CanvasAnnotation *annotation = nullptr;
    void update_annotation();
    std::optional<UUID> map_port_dialog();
};
} // namespace horizon
