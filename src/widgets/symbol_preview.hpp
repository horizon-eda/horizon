#pragma once
#include <gtkmm.h>
#include <set>
#include "common/common.hpp"
#include "util/uuid.hpp"

namespace horizon {
class SymbolPreview : public Gtk::Box {
public:
    SymbolPreview(class Pool &pool);

    void load(const UUID &uu);

private:
    UUID symbol;
    class Pool &pool;
    class PreviewCanvas *canvas_symbol = nullptr;

    Gtk::RadioButton *rb_normal = nullptr;
    Gtk::RadioButton *rb_mirrored = nullptr;

    std::array<Gtk::RadioButton *, 4> rb_angles;

    void update(bool fit = false);
};
} // namespace horizon
