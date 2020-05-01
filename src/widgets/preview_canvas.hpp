#pragma once
#include "util/uuid.hpp"
#include "util/placement.hpp"
#include <gtkmm.h>

namespace horizon {
class PreviewCanvas : public Gtk::Overlay {
public:
    PreviewCanvas(class Pool &pool, bool layered);
    void load(ObjectType ty, const UUID &uu, const Placement &pl = Placement(), bool fit = true);
    void load_symbol(const UUID &uu, const Placement &pl = Placement(), bool fit = true, const UUID &uu_part = UUID(),
                     const UUID &uu_gate = UUID());
    void load(class Package &pkg, bool fit = true);
    class CanvasGL &get_canvas();
    void set_has_scale(bool has_scale);
    void clear();

private:
    class Pool &pool;
    class CanvasGL *canvas = nullptr;
    Gtk::Label *scale_label = nullptr;
    Gtk::Frame *frame = nullptr;
    class ScaleBar *scale_bar = nullptr;
    bool update_scale();
    void update_scale_deferred();
    sigc::connection timeout_connection;
};
} // namespace horizon
