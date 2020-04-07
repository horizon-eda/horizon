#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "util/uuid.hpp"
#include "tool_window.hpp"
#include "widgets/spin_button_dim.hpp"
#include "core/tools/tool_generate_silkscreen.hpp"

namespace horizon {
class GenerateSilkscreenWindow : public ToolWindow {
public:
    GenerateSilkscreenWindow(Gtk::Window *parent, class ImpInterface *intf, class ToolSettings *stg);

    void update();

private:
    void load_defaults();

    ToolGenerateSilkscreen::Settings *settings;

    Gtk::Entry *entry_prefix = nullptr;
    SpinButtonDim *sp_silk = nullptr;
    SpinButtonDim *sp_pad = nullptr;
};
} // namespace horizon
