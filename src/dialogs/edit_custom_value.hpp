#pragma once
#include <gtkmm.h>

namespace horizon {
class EditCustomValueDialog : public Gtk::Dialog {
public:
    EditCustomValueDialog(Gtk::Window *parent, class SchematicSymbol &sym);

private:
    class SchematicSymbol &sym;
    Gtk::TextView *view = nullptr;
    Gtk::Label *preview_label = nullptr;
    void update_preview();
};
} // namespace horizon
