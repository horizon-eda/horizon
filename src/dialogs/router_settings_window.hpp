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

private:
    ToolRouteTrackInteractive::Settings &settings;
};
} // namespace horizon
