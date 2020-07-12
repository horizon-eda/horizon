#pragma once
#include "core/tool.hpp"
#include "schematic/power_symbol.hpp"
#include "tool_place_junction.hpp"
#include <forward_list>

namespace horizon {

class ToolPlacePowerSymbol : public ToolPlaceJunction {
public:
    ToolPlacePowerSymbol(IDocument *c, ToolID tid);
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
    bool update_attached(const ToolArgs &args) override;
    bool check_line(LineNet *li) override;
    PowerSymbol *sym = nullptr;
    std::forward_list<PowerSymbol *> symbols_placed;
    Net *net = nullptr;

private:
    bool do_merge(Net *other);
};
} // namespace horizon
