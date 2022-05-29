#pragma once
#include <gtkmm.h>
#include "util/scroll_direction.hpp"

namespace horizon {
void preferences_provider_attach_canvas(class CanvasGL *ca, bool layer);
void preferences_apply_to_canvas(class CanvasGL *ca, const class Preferences &prefs);
void preferences_apply_appearance(const class Preferences &prefs);
struct DeviceInfo {
    GdkInputSource src;
    ScrollDirection zoom;
    ScrollDirection pan;
};
DeviceInfo preferences_get_device_info(GdkDevice *dev, const class InputDevicesPrefs &prefs);
} // namespace horizon
