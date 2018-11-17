#pragma once
#include "pool/symbol.hpp"
#include <gtkmm.h>
#include <utility>
#include "util/changeable.hpp"

namespace horizon {
class SymbolPreviewBox : public Gtk::Box, public Changeable {
public:
    SymbolPreviewBox(const std::pair<int, bool> &view);
    void update(const Symbol &sym);
    void zoom_to_fit();
    std::map<std::tuple<int, bool, UUID>, Placement> get_text_placements() const;
    void set_text_placements(const std::map<std::tuple<int, bool, UUID>, Placement> &p);
    void set_canvas_appearance(const class Appearance &a);
    void set_can_load(bool can_load);

    typedef sigc::signal<void, int, bool> type_signal_load;
    type_signal_load signal_load()
    {
        return s_signal_load;
    }

private:
    class CanvasGL *canvas = nullptr;
    Gtk::Button *set_button = nullptr;
    Gtk::Button *load_button = nullptr;
    const std::pair<int, bool> view;
    Symbol symbol;
    std::map<UUID, Placement> text_placements;
    void set_placements();
    void clear_placements();
    type_signal_load s_signal_load;
};
} // namespace horizon
