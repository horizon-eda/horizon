#pragma once
#include "canvas/triangle.hpp"
#include "canvas/selectables.hpp"
#include "canvas/snap_filter.hpp"
#include "dialogs/dialogs.hpp"
#include "core/tool_data.hpp"

namespace horizon {
class ImpInterface {
public:
    ImpInterface(class ImpBase *i);
    Dialogs dialogs;
    void tool_bar_set_tip(const std::string &s);
    void tool_bar_set_tool_name(const std::string &s);
    void tool_bar_flash(const std::string &s);
    void part_placed(const UUID &uu);
    void set_work_layer(int layer);
    int get_work_layer();
    void set_layer_display(int layer, const class LayerDisplay &ld);
    const LayerDisplay &get_layer_display(int layer) const;
    void set_no_update(bool v);
    void canvas_update();
    class CanvasGL *get_canvas();
    uint64_t get_grid_spacing();
    void tool_update_data(std::unique_ptr<ToolData> data);

    void update_highlights();
    std::set<ObjectRef> &get_highlights();

    void set_snap_filter(const std::set<SnapFilter> &filter);
    void set_snap_filter_from_selection(const std::set<SelectableRef> &sel);

private:
    class ImpBase *imp;
};
} // namespace horizon
