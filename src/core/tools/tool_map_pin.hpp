#pragma once
#include "core/tool.hpp"

namespace horizon {
class ToolMapPin : public ToolBase {
public:
    ToolMapPin(IDocument *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    ~ToolMapPin();

private:
    std::vector<std::pair<const class Pin *, bool>> pins;
    unsigned int pin_index = 0;
    class SymbolPin *pin = nullptr;
    SymbolPin *pin_last = nullptr;
    SymbolPin *pin_last2 = nullptr;
    void create_pin(const UUID &uu);
    bool can_autoplace() const;
    void update_tip();
    class CanvasAnnotation *annotation = nullptr;
    void update_annotation();
};
} // namespace horizon
