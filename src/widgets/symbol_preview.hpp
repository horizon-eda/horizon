#pragma once
#include <gtkmm.h>
#include <set>
#include "common/common.hpp"
#include "util/uuid.hpp"
#include "util/pool_goto_provider.hpp"

namespace horizon {
class SymbolPreview : public Gtk::Box, public PoolGotoProvider {
public:
    SymbolPreview(class IPool &pool);

    void load(const UUID &uu);

private:
    UUID symbol;
    class IPool &pool;
    class PreviewCanvas *canvas_symbol = nullptr;

    Gtk::RadioButton *rb_normal = nullptr;
    Gtk::RadioButton *rb_mirrored = nullptr;

    std::array<Gtk::RadioButton *, 4> rb_angles;

    Gtk::Label *unit_label = nullptr;

    void update(bool fit = false);
};
} // namespace horizon
