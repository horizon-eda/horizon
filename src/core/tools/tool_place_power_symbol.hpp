#pragma once
#include "core/tool.hpp"
#include "tool_place_junction_schematic.hpp"
#include <forward_list>

namespace horizon {

class ToolPlacePowerSymbol : public ToolPlaceJunctionSchematic {
public:
    using ToolPlaceJunctionSchematic::ToolPlaceJunctionSchematic;

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
    std::forward_list<PowerSymbol *> symbols_placed;
    Net *net = nullptr;

private:
    bool do_merge(Net *other);
};
} // namespace horizon
