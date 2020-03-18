#include "preferences_provider.hpp"
#include "preferences.hpp"
#include "canvas/canvas_gl.hpp"

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
    ca->touchpad_pan = PreferencesProvider::get_prefs().zoom.touchpad_pan;
    PreferencesProvider::get().signal_changed().connect(sigc::track_obj(
            [ca, a] {
                ca->set_appearance(*a);
                ca->smooth_zoom = PreferencesProvider::get_prefs().zoom.smooth_zoom_2d;
                ca->touchpad_pan = PreferencesProvider::get_prefs().zoom.touchpad_pan;
            },
            *ca));
}

} // namespace horizon
