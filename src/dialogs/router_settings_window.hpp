#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "util/uuid.hpp"
#include "tool_window.hpp"
#include "widgets/spin_button_dim.hpp"
#include "core/tools/tool_route_track_interactive.hpp"

namespace horizon {
class RouterSettingsWindow : public ToolWindow {
public:
    RouterSettingsWindow(Gtk::Window *parent, class ImpInterface *intf, class ToolSettings &stg);
    void set_is_routing(bool is_routing);

private:
    using Mode = ToolRouteTrackInteractive::Settings::Mode;
    using CornerMode = ToolRouteTrackInteractive::Settings::CornerMode;

    ToolRouteTrackInteractive::Settings &settings;
    Gtk::Switch *drc_switch = nullptr;
    Gtk::ComboBoxText *mode_combo = nullptr;
    Gtk::ComboBoxText *corner_mode_combo = nullptr;
    void update_drc();
};
} // namespace horizon
