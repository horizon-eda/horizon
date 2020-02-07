#pragma once
#include "core/tool.hpp"

namespace horizon {

class ToolDrawConnectionLine : public ToolBase {
public:
    ToolDrawConnectionLine(IDocument *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;

private:
    class Junction *temp_junc = 0;
    class ConnectionLine *temp_line = 0;
    class ConnectionLine *temp_line_last = 0;
    class ConnectionLine *temp_line_last2 = 0;
    class Net *temp_net = nullptr;

    void update_tip();
};
} // namespace horizon
