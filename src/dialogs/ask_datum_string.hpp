#pragma once
#include "widgets/text_editor.hpp"
namespace horizon {
class AskDatumStringDialog : public Gtk::Dialog {
public:
    AskDatumStringDialog(Gtk::Window *parent, const std::string &label, TextEditor::Lines mode);
    void set_text(const std::string &text);
    std::string get_text() const;

private:
    TextEditor *editor = nullptr;
};
} // namespace horizon
