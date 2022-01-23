#pragma once
#include <gtkmm.h>
#include "tool_window.hpp"

namespace horizon {
class EditTextWindow : public ToolWindow {
public:
    EditTextWindow(Gtk::Window *parent, ImpInterface *intf, class Text &text, bool use_ok);
    void focus_text();
    void focus_size();
    void focus_width();
    void set_dims(uint64_t size, uint64_t width);

private:
    Text &text;

    class TextEditor *editor = nullptr;
    class SpinButtonDim *sp_size = nullptr;
    class SpinButtonDim *sp_width = nullptr;
    Gtk::ComboBoxText *combo_font = nullptr;
};
} // namespace horizon
