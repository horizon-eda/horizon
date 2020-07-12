#pragma once
#include "canvas/object_ref.hpp"
#include "canvas/selectables.hpp"
#include "canvas/snap_filter.hpp"
#include "dialogs/dialogs.hpp"
#include "core/tool_data.hpp"
#include <sigc++/sigc++.h>
#include "util/action_label.hpp"

namespace horizon {
enum class InToolActionID;
class ImpInterface {
public:
    ImpInterface(class ImpBase *i);
    Dialogs dialogs;
    void tool_bar_set_tip(const std::string &s);
    void tool_bar_set_tool_name(const std::string &s);
    void tool_bar_flash(const std::string &s);
    void tool_bar_set_actions(const std::vector<ActionLabelInfo> &labels);

    void part_placed(const UUID &uu);
    void set_work_layer(int layer);
    int get_work_layer();
    void set_layer_display(int layer, const class LayerDisplay &ld);
    const LayerDisplay &get_layer_display(int layer) const;
    void set_no_update(bool v);
    void canvas_update();
    class CanvasGL *get_canvas();
    Coordi get_grid_spacing() const;
    void tool_update_data(std::unique_ptr<ToolData> data);

    void update_highlights();
    std::set<ObjectRef> &get_highlights();

    void set_snap_filter(const std::set<SnapFilter> &filter);
    void set_snap_filter_from_selection(const std::set<SelectableRef> &sel);
    uint64_t get_length_tuning_ref() const;

    typedef sigc::signal<uint64_t> type_signal_request_length_tuning_ref;
    type_signal_request_length_tuning_ref signal_request_length_tuning_ref()
    {
        return s_signal_request_length_tuning_ref;
    }

private:
    class ImpBase *imp;
    type_signal_request_length_tuning_ref s_signal_request_length_tuning_ref;
};
} // namespace horizon
