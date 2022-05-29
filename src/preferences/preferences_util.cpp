#include "preferences_provider.hpp"
#include "preferences.hpp"
#include "canvas/canvas_gl.hpp"
#include "preferences_util.hpp"

namespace horizon {

void preferences_provider_attach_canvas(CanvasGL *ca, bool layer)
{
    const Appearance *a = nullptr;
    if (layer) {
        a = &PreferencesProvider::get_prefs().canvas_layer.appearance;
    }
    else {
        a = &PreferencesProvider::get_prefs().canvas_non_layer.appearance;
    }
    ca->set_appearance(*a);
    preferences_apply_to_canvas(ca, PreferencesProvider::get_prefs());
    PreferencesProvider::get().signal_changed().connect(sigc::track_obj(
            [ca, a] {
                ca->set_appearance(*a);
                preferences_apply_to_canvas(ca, PreferencesProvider::get_prefs());
            },
            *ca));
}

void preferences_apply_to_canvas(class CanvasGL *ca, const class Preferences &prefs)
{
    ca->smooth_zoom = prefs.zoom.smooth_zoom_2d;
    ca->touchpad_pan = prefs.zoom.touchpad_pan;
    ca->zoom_base = 1 + (prefs.zoom.zoom_factor / 100);
    ca->input_devices_prefs = prefs.input_devices.prefs;
}

void preferences_apply_appearance(const Preferences &prefs)
{
    Gtk::Settings::get_default()->property_gtk_application_prefer_dark_theme().set_value(prefs.appearance.dark_theme);
}

static ScrollDirection get_scroll_direction(bool invert)
{
    if (invert)
        return ScrollDirection::REVERSED;
    else
        return ScrollDirection::NORMAL;
}

DeviceInfo preferences_get_device_info(GdkDevice *dev, const class InputDevicesPrefs &prefs)
{
    auto src = gdk_device_get_source(dev);
    std::string name = gdk_device_get_name(dev);
    if (prefs.devices.count(name)) {
        auto &d = prefs.devices.at(name);
        if (d.type == InputDevicesPrefs::Device::Type::TOUCHPAD)
            src = GDK_SOURCE_TOUCHPAD;
        else if (d.type == InputDevicesPrefs::Device::Type::TRACKPOINT)
            src = GDK_SOURCE_TRACKPOINT;
        return {src, get_scroll_direction(d.invert_zoom), get_scroll_direction(d.invert_pan)};
    }
    return {src, ScrollDirection::NORMAL, ScrollDirection::NORMAL};
}

} // namespace horizon
