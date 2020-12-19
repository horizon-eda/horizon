#pragma once
#include "core/tool.hpp"
#include "tool_place_junction_schematic.hpp"
#include <forward_list>

namespace horizon {

class ToolPlaceBusLabel : public ToolPlaceJunctionSchematic {
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

private:
    void create_attached() override;
    void delete_attached() override;
    bool begin_attached() override;
    bool update_attached(const ToolArgs &args) override;
    bool check_line(class LineNet *li) override;
    class BusLabel *la = nullptr;
    Orientation last_orientation = Orientation::RIGHT;
    std::forward_list<class BusLabel *> labels_placed;
    class Bus *bus = nullptr;
};
} // namespace horizon
