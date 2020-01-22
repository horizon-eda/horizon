#pragma once
#include <gtkmm.h>
#include <set>
#include "common/common.hpp"
#include "util/uuid.hpp"
#include "preview_base.hpp"
#include "generic_combo_box.hpp"


namespace horizon {
class EntityPreview : public Gtk::Box, public PreviewBase {
public:
    EntityPreview(class Pool &pool, bool show_goto = true);

    void load(const class Entity *entity);
    void load(const class Part *part);
    void clear();

private:
    void load(const class Entity *entity, const class Part *part);
    class Pool &pool;
    const class Entity *entity = nullptr;
    const class Part *part = nullptr;
    class PreviewCanvas *canvas_symbol = nullptr;
    GenericComboBox<UUID> *combo_gate = nullptr;
    GenericComboBox<UUID> *combo_symbol = nullptr;
    Gtk::Button *goto_symbol_button = nullptr;
    Gtk::Button *goto_unit_button = nullptr;

    void handle_gate_sel();
    void handle_symbol_sel();
};
} // namespace horizon
