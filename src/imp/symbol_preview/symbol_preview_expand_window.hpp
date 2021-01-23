#pragma once
#include "util/placement.hpp"
#include "util/uuid.hpp"
#include "util/changeable.hpp"
#include "util/window_state_store.hpp"
#include <gtkmm.h>

namespace horizon {
class SymbolPreviewExpandWindow : public Gtk::Window {
public:
    SymbolPreviewExpandWindow(Gtk::Window *parent, const class Symbol &sym);
    void update();
    void set_canvas_appearance(const class Appearance &a);
    void zoom_to_fit();

private:
    const class Symbol &sym;
    class CanvasGL *canvas = nullptr;
    Gtk::SpinButton *sp_expand = nullptr;
    Gtk::Box *box2 = nullptr;

    WindowStateStore state_store;
};
} // namespace horizon
