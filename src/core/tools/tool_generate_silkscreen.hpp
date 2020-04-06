#pragma once
#include "core/tool.hpp"
#include "pool/package.hpp"
#include <set>

namespace horizon {

class ToolGenerateSilkscreen : public ToolBase {
public:
    ToolGenerateSilkscreen(IDocument *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    bool is_specific() override
    {
        return false;
    }
    bool handles_esc() override
    {
        return true;
    }

private:
    bool select_polygon();
    void update_tip();
    void clear_silkscreen();

    enum class Adjust { SILK, PAD };

    Adjust adjust = Adjust::SILK;
    const Polygon *pp;
    int64_t expand_silk;
    int64_t expand_pad;
    bool first_update;
    ClipperLib::Path path_pkg;
    ClipperLib::Paths pads;
};
} // namespace horizon
