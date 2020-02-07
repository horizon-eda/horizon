#pragma once
#include "core/tool.hpp"
#include "tool_helper_move.hpp"

namespace horizon {

class ToolImportDXF : public ToolHelperMove {
public:
    ToolImportDXF(IDocument *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args);
    ToolResponse update(const ToolArgs &args);
    bool can_begin();

private:
    std::set<class Line *> lines;
    std::set<class Arc *> arcs;
    int64_t width = 0;
};
} // namespace horizon
