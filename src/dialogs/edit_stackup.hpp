#pragma once
#include <gtkmm.h>

namespace horizon {
class EditStackupDialog : public Gtk::Dialog {
    friend class StackupLayerEditor;

public:
    EditStackupDialog(Gtk::Window *parent, class IDocumentBoard &core);

private:
    class IDocumentBoard &core;
    class Board &board;
    Gtk::ListBox *lb = nullptr;
    Gtk::SpinButton *sp_n_inner_layers = nullptr;
    void ok_clicked();
    void update_layers();
    std::map<std::pair<int, bool>, uint64_t> saved;
    Glib::RefPtr<Gtk::SizeGroup> sg_layer_name;
};
} // namespace horizon
