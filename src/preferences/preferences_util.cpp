#include "preferences_provider.hpp"
#include "preferences.hpp"
#include "canvas/canvas_gl.hpp"

namespace horizon {

class DestroyData {
public:
    sigc::connection conn;
};

static void *canvas_destroy(void *p)
{
    auto d = reinterpret_cast<DestroyData *>(p);
    d->conn.disconnect();
    delete d;
    return nullptr;
}

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
    auto d = new DestroyData;
    d->conn = PreferencesProvider::get().signal_changed().connect([ca, a] { ca->set_appearance(*a); });
    ca->Gtk::GLArea::add_destroy_notify_callback(d, &canvas_destroy);
}

} // namespace horizon
