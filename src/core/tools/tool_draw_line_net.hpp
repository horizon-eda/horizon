#pragma once
#include "core/tool.hpp"
#include "tool_helper_merge.hpp"
#include "tool_helper_draw_net_setting.hpp"

namespace horizon {

class ToolDrawLineNet : public ToolHelperMerge, public ToolHelperDrawNetSetting {
public:
    ToolDrawLineNet(IDocument *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;

    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::LMB,
                I::CANCEL,
                I::RMB,
                I::PLACE_JUNCTION,
                I::COMMIT,
                I::POSTURE,
                I::ARBITRARY_ANGLE_MODE,
                I::NET_LABEL_SIZE_INC,
                I::NET_LABEL_SIZE_DEC,
                I::ENTER_SIZE,
                I::ROTATE,
                I::TOGGLE_NET_LABEL,
        };
    }

private:
    class SchematicJunction *temp_junc_head = 0;
    SchematicJunction *temp_junc_mid = 0;
    class LineNet *temp_line_head = 0;
    class LineNet *temp_line_mid = 0;
    class NetLabel *net_label = nullptr;
    enum class BendMode { XY, YX, ARB };
    BendMode bend_mode = BendMode::XY;
    void move_temp_junc(const Coordi &c);
    void update_tip();
    void restart(const Coordi &c);

    class Component *component_floating = nullptr;
    UUIDPath<2> connpath_floating;
    class SymbolPin *pin_start = nullptr;

    SchematicJunction *make_temp_junc(const Coordi &c);
    void apply_settings() override;
    void set_snap_filter();
    LineNet *insert_line_net();
};
} // namespace horizon
