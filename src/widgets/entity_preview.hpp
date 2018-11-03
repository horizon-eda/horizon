#pragma once
#include <gtkmm.h>
#include <set>
#include "common/common.hpp"
#include "util/uuid.hpp"
#include "preview_base.hpp"

namespace horizon {
class EntityPreview : public Gtk::Box, public PreviewBase {
public:
    EntityPreview(class Pool &pool, bool show_goto = true);

    void load(const class Entity *entity);
    void load(const class Part *part);
    void load(nullptr_t n);

private:
    void load(const class Entity *entity, const class Part *part);
    class Pool &pool;
    const class Entity *entity = nullptr;
    const class Part *part = nullptr;
    class PreviewCanvas *canvas_symbol = nullptr;
    Gtk::ComboBoxText *combo_gate = nullptr;
    Gtk::ComboBoxText *combo_symbol = nullptr;

    void handle_gate_sel();
    void handle_symbol_sel();
};
} // namespace horizon
