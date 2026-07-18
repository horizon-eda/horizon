#pragma once
#include "tool_place_junction_schematic.hpp"
#include "tool_helper_merge.hpp"

namespace horizon {

class ToolPlacePowerSymbol : public ToolPlaceJunctionSchematic, public ToolHelperMerge {
public:
    ToolPlacePowerSymbol(IDocument *c, ToolID tid);

    ToolResponse begin(const ToolArgs &args) override;

    bool can_begin() override;
    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::LMB, I::CANCEL, I::RMB, I::ROTATE, I::MIRROR,
        };
    }

protected:
    void create_attached() override;
    void delete_attached() override;
    bool begin_attached() override;
    bool junction_placed() override;
    bool update_attached(const ToolArgs &args) override;
    bool check_line(LineNet *li) override;
    class PowerSymbol *sym = nullptr;
    Net *net = nullptr;
};
} // namespace horizon
