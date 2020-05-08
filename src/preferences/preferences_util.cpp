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
}

} // namespace horizon
