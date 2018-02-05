#pragma once
#include "util/placement.hpp"
#include "util/uuid.hpp"
#include <gtkmm.h>

namespace horizon {
class SymbolPreviewWindow : public Gtk::Window {
public:
    SymbolPreviewWindow(Gtk::Window *parent);
    void update(const class Symbol &sym);
    std::map<std::tuple<int, bool, UUID>, Placement> get_text_placements() const;
    void set_text_placements(const std::map<std::tuple<int, bool, UUID>, Placement> &p);

private:
    std::map<std::pair<int, bool>, class SymbolPreviewBox *> previews;
};
} // namespace horizon
