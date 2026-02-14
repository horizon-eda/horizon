#pragma once

#include "tool_window.hpp"
#include "widgets/spin_button_dim.hpp"


namespace horizon {
class EditTableWindow : public ToolWindow {
public:
    EditTableWindow(Window *parent, ImpInterface *intf, class Table &table, bool use_ok);

    void focus_n_rows();

private:
    Table &table;

    Gtk::SpinButton *sp_n_rows = nullptr;
    Gtk::SpinButton *sp_n_columns = nullptr;
    SpinButtonDim *sp_text_size = nullptr;
    SpinButtonDim *sp_line_width = nullptr;
    SpinButtonDim *sp_padding = nullptr;
    Gtk::ComboBoxText *combo_font = nullptr;
    Gtk::Grid *cell_grid = nullptr;

    void rebuild_cell_grid();
};
} // namespace horizon