#pragma once
#include <gtkmm.h>
#include "canvas/selectables.hpp"

namespace horizon {
class ClipboardHandler {
public:
    ClipboardHandler(class ClipboardBase &clip);
    void copy(std::set<SelectableRef> selection, const Coordi &cursor_pos);

private:
    ClipboardBase &clip;
    std::string clipboard_serialized;
    void on_clipboard_get(Gtk::SelectionData &selection_data, guint /* info */);
    void on_clipboard_clear();
};
} // namespace horizon
