#pragma once
#include "core.hpp"
#include <forward_list>
#include <map>

namespace horizon {

class ToolPlaceRefdesAndValue : public ToolBase {
public:
    ToolPlaceRefdesAndValue(Core *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;

private:
    Text *text_refdes = nullptr;
    Text *text_value = nullptr;

    void update_texts(const Coordi &c);
};
} // namespace horizon
