#pragma once
#include "util/placement.hpp"
#include "util/uuid.hpp"
#include "util/changeable.hpp"
#include <gtkmm.h>

namespace horizon {
class SymbolPreviewWindow : public Gtk::Window, public Changeable {
public:
    SymbolPreviewWindow(Gtk::Window *parent);
    void update(const class Symbol &sym);
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
    std::map<std::pair<int, bool>, class SymbolPreviewBox *> previews;
    type_signal_load s_signal_load;
};
} // namespace horizon
